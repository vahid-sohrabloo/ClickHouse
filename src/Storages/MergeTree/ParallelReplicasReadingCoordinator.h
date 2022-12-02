#pragma once

#include <memory>
#include <Storages/MergeTree/RequestResponse.h>


namespace DB
{

class ParallelReplicasReadingCoordinator
{
public:
    class ImplInterface;

    explicit ParallelReplicasReadingCoordinator(size_t replicas_count_);
    ~ParallelReplicasReadingCoordinator();

    void setMode(CoordinationMode mode);
    void initialize();
    void handleInitialAllRangesAnnouncement(InitialAllRangesAnnouncement);
    ParallelReadResponse handleRequest(ParallelReadRequest request);

private:
    CoordinationMode mode{CoordinationMode::Default};
    size_t replicas_count{0};
    std::atomic<bool> initialized{false};
    std::unique_ptr<ImplInterface> pimpl;
};

using ParallelReplicasReadingCoordinatorPtr = std::shared_ptr<ParallelReplicasReadingCoordinator>;

}
