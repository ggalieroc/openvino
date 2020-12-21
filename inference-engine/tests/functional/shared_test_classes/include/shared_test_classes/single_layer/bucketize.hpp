// Copyright (C) 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <tuple>
#include <string>

#include "ngraph_functions/builders.hpp"
#include "ngraph_functions/utils/ngraph_helpers.hpp"

#include "shared_test_classes/base/layer_test_utils.hpp"

namespace LayerTestsDefinitions {

using bucketizeParamsTuple = std::tuple<
    InferenceEngine::SizeVector,    // Data shape
    InferenceEngine::SizeVector,    // Buckets shape
    bool,                           // Right edge of interval
    InferenceEngine::Precision,     // Input precision
    InferenceEngine::Precision,     // Output precision
    std::string>;                   // Device name

class BucketizeLayerTest : public testing::WithParamInterface<bucketizeParamsTuple>,
                           virtual public LayerTestsUtils::LayerTestsCommon {
public:
    static std::string getTestCaseName(testing::TestParamInfo<bucketizeParamsTuple> obj);
protected:
    void SetUp() override;
};

} // namespace LayerTestsDefinitions
