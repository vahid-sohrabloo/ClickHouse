#include <Storages/MergeTree/MergeTreeParallelReplicasSelectProcessor.h>

#include <Storages/MergeTree/IMergeTreeReader.h>
#include <Storages/MergeTree/MergeTreeReadPoolParallelReplicas.h>
#include <Interpreters/Context.h>


namespace DB
{

MergeTreeParallelReplicasSelectProcessor::MergeTreeParallelReplicasSelectProcessor(
    size_t thread_,
    MergeTreeReadPoolParallelReplicasPtr pool_,
    UInt64 max_block_size_rows_,
    size_t preferred_block_size_bytes_,
    size_t preferred_max_column_in_block_size_bytes_,
    const MergeTreeData & storage_,
    const StorageSnapshotPtr & storage_snapshot_,
    bool use_uncompressed_cache_,
    const PrewhereInfoPtr & prewhere_info_,
    ExpressionActionsSettings actions_settings,
    const MergeTreeReaderSettings & reader_settings_,
    const Names & virt_column_names_)
    :
    IMergeTreeSelectAlgorithm{
        pool_->getHeader(), storage_, storage_snapshot_, prewhere_info_, std::move(actions_settings), max_block_size_rows_,
        preferred_block_size_bytes_, preferred_max_column_in_block_size_bytes_,
        reader_settings_, use_uncompressed_cache_, virt_column_names_},
    thread{thread_},
    pool{pool_}
{
}

/// Requests read task from MergeTreeReadPool and signals whether it got one
bool MergeTreeParallelReplicasSelectProcessor::getNewTaskImpl()
{
    task = pool->getTask();
    return static_cast<bool>(task);
}


void MergeTreeParallelReplicasSelectProcessor::finalizeNewTask()
{
    const std::string part_name = task->data_part->isProjectionPart() ? task->data_part->getParentPart()->name : task->data_part->name;

    /// Allows pool to reduce number of threads in case of too slow reads.
    auto profile_callback = [this](ReadBufferFromFileBase::ProfileInfo info_) { pool->profileFeedback(info_); };
    const auto & metadata_snapshot = storage_snapshot->metadata;

    IMergeTreeReader::ValueSizeMap value_size_map;

    if (!reader)
    {
        if (use_uncompressed_cache)
            owned_uncompressed_cache = storage.getContext()->getUncompressedCache();
        owned_mark_cache = storage.getContext()->getMarkCache();
    }
    else if (part_name != last_readed_part_name)
    {
        value_size_map = reader->getAvgValueSizeHints();
    }

    const bool init_new_readers = !reader || part_name != last_readed_part_name;
    if (init_new_readers)
    {
        initializeMergeTreeReadersForPart(task->data_part, task->task_columns, metadata_snapshot,
            task->mark_ranges, value_size_map, profile_callback);
    }

    last_readed_part_name = part_name;
}


void MergeTreeParallelReplicasSelectProcessor::finish()
{
    reader.reset();
    pre_reader_for_step.clear();
}


MergeTreeParallelReplicasSelectProcessor::~MergeTreeParallelReplicasSelectProcessor() = default;

}
