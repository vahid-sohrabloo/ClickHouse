#pragma once

#include <Storages/MergeTree/MergeTreeBaseSelectProcessor.h>

#include <memory>

namespace DB
{

class MergeTreeReadPoolParallelReplicas;
using MergeTreeReadPoolParallelReplicasPtr = std::shared_ptr<MergeTreeReadPoolParallelReplicas>;

class MergeTreeParallelReplicasSelectProcessor final : public IMergeTreeSelectAlgorithm
{
public:
    MergeTreeParallelReplicasSelectProcessor(
        size_t thread_,
        MergeTreeReadPoolParallelReplicasPtr pool_,
        UInt64 max_block_size_,
        size_t preferred_block_size_bytes_,
        size_t preferred_max_column_in_block_size_bytes_,
        const MergeTreeData & storage_,
        const StorageSnapshotPtr & storage_snapshot_,
        bool use_uncompressed_cache_,
        const PrewhereInfoPtr & prewhere_info_,
        ExpressionActionsSettings actions_settings,
        const MergeTreeReaderSettings & reader_settings_,
        const Names & virt_column_names_);

    String getName() const override { return "MergeTreeThread"; }

    ~MergeTreeParallelReplicasSelectProcessor() override;

protected:
    /// Requests read task from MergeTreeReadPool and signals whether it got one
    bool getNewTaskImpl() override;

    void finalizeNewTask() override;

    void finish() override;

private:
    /// "thread" index (there are N threads and each thread is assigned index in interval [0..N-1])
    [[ maybe_unused ]] size_t thread;

    MergeTreeReadPoolParallelReplicasPtr pool;

    /// Last part read in this thread
    std::string last_readed_part_name;
};

}
