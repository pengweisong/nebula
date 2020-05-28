/* Copyright (c) 2020 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License,
 * attached with Common Clause Condition 1.0, found in the LICENSES directory.
 */

#include "base/Base.h"
#include <gtest/gtest.h>
#include "fs/TempDir.h"
#include "kvstore/wal/FileBasedWal.h"
#include "kvstore/wal/WalFileIterator.h"

namespace nebula {
namespace wal {

using nebula::fs::FileUtils;
using nebula::fs::TempDir;

TEST(WalFileIter, SimpleReadTest) {
    FileBasedWalPolicy policy;
    TempDir walDir("/tmp/testWal.XXXXXX");

    auto wal = FileBasedWal::getWal(walDir.path(),
                                    "",
                                    policy,
                                    [](LogID, TermID, ClusterID, const std::string&) {
                                        return true;
                                    });
    EXPECT_EQ(0, wal->lastLogId());

    for (int i = 1; i <= 10; i++) {
        EXPECT_TRUE(
            wal->appendLog(i /*id*/, 1 /*term*/, 0 /*cluster*/,
                           folly::stringPrintf("Test string %02d", i)));
    }
    EXPECT_EQ(10, wal->lastLogId());

    {
        auto it = std::make_unique<WalFileIterator>(wal, 1, 10);
        LogID id = 1;
        while (it->valid()) {
            EXPECT_EQ(id, it->logId());
            EXPECT_EQ(folly::stringPrintf("Test string %02ld", id),
                      it->logMsg());
            ++(*it);
            ++id;
        }
        EXPECT_EQ(11, id);
    }
}

TEST(WalFileIter, MultiFilesReadTest) {
    FileBasedWalPolicy policy;
    policy.fileSize = 1024;
    TempDir walDir("/tmp/testWal.XXXXXX");

    auto wal = FileBasedWal::getWal(walDir.path(),
                                    "",
                                    policy,
                                    [](LogID, TermID, ClusterID, const std::string&) {
                                        return true;
                                    });
    EXPECT_EQ(0, wal->lastLogId());

    for (int i = 1; i <= 50000; i++) {
        EXPECT_TRUE(
            wal->appendLog(i /*id*/, 1 /*term*/, 0 /*cluster*/,
                           folly::stringPrintf("Test string %02d", i)));
    }
    LOG(INFO) << "Total wals number " << wal->walFiles_.size();
    EXPECT_EQ(50000, wal->lastLogId());
    EXPECT_LT(10, wal->walFiles_.size());
    {
        auto it = std::make_unique<WalFileIterator>(wal, 1, 50000);
        LogID id = 1;
        while (it->valid()) {
            EXPECT_EQ(id, it->logId());
            EXPECT_EQ(folly::stringPrintf("Test string %02ld", id),
                      it->logMsg());
            ++(*it);
            ++id;
        }
        EXPECT_EQ(50001, id);
    }
    {
        auto it = std::make_unique<WalFileIterator>(wal, 10000, 50000);
        LogID id = 10000;
        while (it->valid()) {
            EXPECT_EQ(id, it->logId());
            EXPECT_EQ(folly::stringPrintf("Test string %02ld", id),
                      it->logMsg());
            ++(*it);
            ++id;
        }
        EXPECT_EQ(50001, id);
    }
    {
        auto it = std::make_unique<WalFileIterator>(wal, 45000, 50000);
        LogID id = 45000;
        while (it->valid()) {
            EXPECT_EQ(id, it->logId());
            EXPECT_EQ(folly::stringPrintf("Test string %02ld", id),
                      it->logMsg());
            ++(*it);
            ++id;
        }
        EXPECT_EQ(50001, id);
    }
}


}  // namespace wal
}  // namespace nebula


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    folly::init(&argc, &argv, true);
    google::SetStderrLogging(google::INFO);

    return RUN_ALL_TESTS();
}

