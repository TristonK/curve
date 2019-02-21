/*
 * Project: curve
 * File Created: Monday, 7th January 2019 10:04:50 pm
 * Author: tongguangxun
 * Copyright (c)￼ 2018 netease
 */
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <fcntl.h>  // NOLINT
#include <string>
#include <iostream>
#include <atomic>
#include <thread>   //NOLINT
#include <chrono>   //NOLINT

#include "src/client/client_common.h"
#include "src/client/libcurve_define.h"
#include "include/client/libcurve_qemu.h"
#include "src/client/libcurve_snapshot.h"
#include "src/client/file_instance.h"
#include "test/client/fake/mock_schedule.h"
#include "test/client/fake/fakeMDS.h"

uint32_t segment_size = 1 * 1024 * 1024 * 1024ul;   // NOLINT
uint32_t chunk_size = 16 * 1024 * 1024;   // NOLINT
std::string metaserver_addr = "127.0.0.1:6666";   // NOLINT

DECLARE_uint64(test_disk_size);
DEFINE_uint32(io_time, 5, "Duration for I/O test");
DEFINE_bool(fake_mds, true, "create fake mds");
DEFINE_bool(create_copysets, false, "create copysets on chunkserver");
DEFINE_bool(verify_io, true, "verify read/write I/O getting done correctly");

bool writeflag = false;
bool readflag = false;
std::mutex writeinterfacemtx;
std::condition_variable writeinterfacecv;
std::mutex interfacemtx;
std::condition_variable interfacecv;

DECLARE_uint64(test_disk_size);

using curve::client::SegmentInfo;
using curve::client::ChunkInfoDetail;
using curve::client::SnapshotClient;
using curve::client::ChunkID;
using curve::client::LogicPoolID;
using curve::client::CopysetID;
using curve::client::ChunkIDInfo;
using curve::client::CopysetInfo_t;
using curve::client::MetaCache;
using curve::client::LogicalPoolCopysetIDInfo;

int main(int argc, char ** argv) {
    // google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::InitGoogleLogging(argv[0]);

    std::string filename = "test.txt";
    /*** init mds service ***/
    FakeMDS mds(filename);
    if (FLAGS_fake_mds) {
        mds.Initialize();
        mds.StartService();
        if (FLAGS_create_copysets) {
            mds.CreateCopysetNode();
        }
    }

    ClientConfigOption_t opt;
    opt.metaserveropt.rpc_timeout_ms = 500;
    opt.metaserveropt.rpc_retry_times = 3;
    opt.metaserveropt.metaaddrvec.push_back("127.0.0.1:6666");
    opt.ioopt.reqschopt.request_scheduler_queue_capacity = 4096;
    opt.ioopt.reqschopt.request_scheduler_threadpool_size = 2;
    opt.ioopt.iosenderopt.failreqopt.client_chunk_op_max_retry = 3;
    opt.ioopt.iosenderopt.failreqopt.client_chunk_op_retry_interval_us = 500;
    opt.ioopt.metacacheopt.get_leader_retry = 3;
    opt.ioopt.iosenderopt.enable_applied_index_read = 1;
    opt.ioopt.iosplitopt.io_split_max_size_kb = 64;
    opt.loglevel = 0;

    SnapshotClient cl;
    if (cl.Init(opt) != 0) {
        LOG(FATAL) << "Fail to init config";
        return -1;
    }

    uint64_t seq = 0;
    if (-1 == cl.CreateSnapShot(filename, &seq)) {
        LOG(ERROR) << "create failed!";
        return -1;
    }

    SegmentInfo seginfo;
    LogicalPoolCopysetIDInfo lpcsIDInfo;
    if (LIBCURVE_ERROR::FAILED == cl.GetSnapshotSegmentInfo(filename,
                                                            &lpcsIDInfo,
                                                            0, 0,
                                                            &seginfo)) {
        LOG(ERROR) << "GetSnapshotSegmentInfo failed!";
        return -1;
    }

    if (LIBCURVE_ERROR::FAILED == cl.GetServerList(lpcsIDInfo.lpid,
                                                    lpcsIDInfo.cpidVec)) {
        LOG(ERROR) << "GetSnapshotSegmentInfo failed!";
        return -1;
    }

    curve::client::FInfo_t sinfo;
    if (-1 == cl.GetSnapShot(filename, seq, &sinfo)) {
        LOG(ERROR) << "ListSnapShot failed!";
        return -1;
    }

    char* readbuf = new char[8192];
    cl.ReadChunkSnapshot(ChunkIDInfo(1, 10000, 1), 1, 0, 8192, static_cast<void*>(readbuf));    // NOLINT
    for (int i = 0; i < 8192; i++) {
        if (readbuf[i] != 1) {
            LOG(ERROR) << "read snap chunk failed!";
        }
    }

    cl.DeleteChunkSnapshot(ChunkIDInfo(1, 10000, 1), 2);

    ChunkInfoDetail *chunkInfo = new ChunkInfoDetail;
    cl.GetChunkInfo(ChunkIDInfo(1, 10000, 1), chunkInfo);
    for (auto iter : chunkInfo->chunkSn) {
        if (iter != 1111) {
            LOG(ERROR) << "chunksn read failed!";
        }
    }
    cl.UnInit();
    return 0;
}