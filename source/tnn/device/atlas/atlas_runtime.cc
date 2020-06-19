// Tencent is pleased to support the open source community by making TNN available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "tnn/device/atlas/atlas_runtime.h"
#include "acl/acl.h"
#include "tnn/core/macro.h"

namespace TNN_NS {

static std::mutex g_mtx;

std::shared_ptr<AtlasRuntime> AtlasRuntime::atlas_runtime_singleton_ = nullptr;
bool AtlasRuntime::enable_increase_count_                            = false;
int AtlasRuntime::ref_count_                                         = 0;
bool AtlasRuntime::init_done_                                        = false;

AtlasRuntime *AtlasRuntime::GetInstance() {
    // don't use DCL
    std::unique_lock<std::mutex> lck(g_mtx);
    if (nullptr == atlas_runtime_singleton_.get()) {
        atlas_runtime_singleton_.reset(new AtlasRuntime());
        ref_count_++;
        enable_increase_count_ = false;
    }

    return atlas_runtime_singleton_.get();
}

void AtlasRuntime::IncreaseRef() {
    std::unique_lock<std::mutex> lck(g_mtx);
    if (enable_increase_count_) {
        ref_count_++;
    }
    enable_increase_count_ = true;
    LOGD("AtlasRuntime::IncreaseRef() count=%d\n", ref_count_);
}

void AtlasRuntime::DecreaseRef() {
    std::unique_lock<std::mutex> lck(g_mtx);
    ref_count_--;
    if (0 == ref_count_) {
        atlas_runtime_singleton_.reset();
        init_done_ = false;
    }
    LOGD("AtlasRuntime::DecreaseRef() count=%d\n", ref_count_);
}

AtlasRuntime::AtlasRuntime() {}

// Init will get platforms info, get devices info, create opencl context.
Status AtlasRuntime::Init() {
    std::unique_lock<std::mutex> lck(g_mtx);

    // only init once.
    if (!init_done_) {
        LOGD("Init Atlas Acl\n");

        aclError ret = aclInit(nullptr);
        if (ret != ACL_ERROR_NONE) {
            LOGE("Init ACL failed!\n");
            return TNNERR_ATLAS_RUNTIME_ERROR;
        }

        init_done_ = true;
    }

    return TNN_OK;
}

AtlasRuntime::~AtlasRuntime() {
    aclError ret = aclFinalize();
    if (ret != ACL_ERROR_NONE) {
        LOGE("DeInit ACL failed!\n");
    }
}

}  // namespace TNN_NS