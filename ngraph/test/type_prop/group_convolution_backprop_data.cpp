//*****************************************************************************
// Copyright 2017-2021 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include "gtest/gtest.h"
#include "ngraph/ngraph.hpp"
#include "util/type_prop.hpp"

using namespace std;
using namespace ngraph;

TEST(type_prop, group_convolution_backprop_data_shape_infer)
{
    const PartialShape data_pshape{1, 16, 6, 6};      // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{2, 8, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});

    EXPECT_EQ(gcbd->get_element_type(), element::f32);
    EXPECT_EQ(gcbd->get_output_shape(0), (Shape{1, 4, 8, 8}));
    EXPECT_EQ(gcbd->get_strides(), (Strides{1, 1}));
    EXPECT_EQ(gcbd->get_dilations(), (Strides{1, 1}));
    EXPECT_EQ(gcbd->get_pads_begin(), (CoordinateDiff{0, 0}));
    EXPECT_EQ(gcbd->get_pads_end(), (CoordinateDiff{0, 0}));
    EXPECT_EQ(gcbd->get_output_padding(), (CoordinateDiff{0, 0}));
    EXPECT_EQ(gcbd->get_auto_pad(), op::PadType::EXPLICIT);
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_with_output_shape_as_const)
{
    const PartialShape data_pshape{1, 16, 5, 5};       // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{1, 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    EXPECT_EQ(gcbd->get_element_type(), element::f32);
    EXPECT_EQ(gcbd->get_output_shape(0), (Shape{1, 2, 3, 3}));
    EXPECT_EQ(gcbd->get_strides(), (Strides{1, 1}));
    EXPECT_EQ(gcbd->get_dilations(), (Strides{1, 1}));
    EXPECT_EQ(gcbd->get_pads_begin(), (CoordinateDiff{2, 2}));
    EXPECT_EQ(gcbd->get_pads_end(), (CoordinateDiff{2, 2}));
    EXPECT_EQ(gcbd->get_output_padding(), (CoordinateDiff{0, 0}));
    EXPECT_EQ(gcbd->get_auto_pad(), op::PadType::SAME_UPPER);
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_with_output_shape_as_param)
{
    const PartialShape data_pshape{1, 16, 5, 5};       // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{1, 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = make_shared<op::Parameter>(element::i64, Shape{2});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    EXPECT_EQ(gcbd->get_element_type(), element::f32);
    EXPECT_EQ(gcbd->get_auto_pad(), op::PadType::SAME_UPPER);
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{1, 2, Dimension::dynamic(), Dimension::dynamic()}));
}

TEST(type_prop,
     group_convolution_backprop_data_shape_infer_with_output_shape_static_ranks_data_nc_dyn)
{
    const PartialShape data_pshape{
        Dimension::dynamic(), Dimension::dynamic(), 5, 5}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{1, 16, 2, 3, 3};     // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(
        gcbd->get_output_partial_shape(0).same_scheme(PartialShape{Dimension::dynamic(), 2, 3, 3}));
}

TEST(type_prop,
     group_convolution_backprop_data_shape_infer_with_output_shape_static_ranks_filters_group_dyn)
{
    const PartialShape data_pshape{Dimension::dynamic(), 16, 5, 5}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(
        gcbd->get_output_partial_shape(0).same_scheme(PartialShape{Dimension::dynamic(), 2, 3, 3}));
}

TEST(
    type_prop,
    group_convolution_backprop_data_shape_infer_with_output_shape_static_ranks_filters_group_cin_dyn)
{
    const PartialShape data_pshape{Dimension::dynamic(), 16, 5, 5}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), Dimension::dynamic(), 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), Dimension::dynamic(), 3, 3}));
}

TEST(
    type_prop,
    group_convolution_backprop_data_shape_infer_with_output_shape_static_ranks_data_cin_filters_group_dyn)
{
    const PartialShape data_pshape{1, Dimension::dynamic(), 5, 5}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(
        gcbd->get_output_partial_shape(0).same_scheme(PartialShape{1, Dimension::dynamic(), 3, 3}));
}

TEST(
    type_prop,
    group_convolution_backprop_data_shape_infer_with_output_shape_static_ranks_filters_group_cout_dyn)
{
    const PartialShape data_pshape{Dimension::dynamic(), 16, 5, 5}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), 16, Dimension::dynamic(), 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), Dimension::dynamic(), 3, 3}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_data_nc_dyn)
{
    const PartialShape data_pshape{
        Dimension::dynamic(), Dimension::dynamic(), 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{4, 5, 2, 3, 3};          // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), 8, 447, 447}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_filters_group_dyn)
{
    const PartialShape data_pshape{1, 20, 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), 5, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(PartialShape{1, 8, 447, 447}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_filters_group_cin_dyn)
{
    const PartialShape data_pshape{Dimension::dynamic(), 20, 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), Dimension::dynamic(), 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), Dimension::dynamic(), 447, 447}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_data_cin_filters_group_dyn)
{
    const PartialShape data_pshape{1, Dimension::dynamic(), 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), 5, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{1, Dimension::dynamic(), 447, 447}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_filters_group_cout_dyn)
{
    const PartialShape data_pshape{1, 20, 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        Dimension::dynamic(), Dimension::dynamic(), 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{1, Dimension::dynamic(), 447, 447}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_data_spatial_dim_dyn)
{
    const PartialShape data_pshape{1, 20, 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{
        4, 5, 2, Dimension::dynamic(), 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{1, 8, Dimension::dynamic(), 447}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_static_ranks_filters_spatial_dim_dyn)
{
    const PartialShape data_pshape{
        Dimension::dynamic(), 20, 224, Dimension::dynamic()}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{4, 5, 2, 3, 3};         // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    const Strides strides{2, 2};
    const Strides dilations{1, 1};
    const CoordinateDiff padding_begin{1, 1};
    const CoordinateDiff padding_end{1, 1};

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, strides, padding_begin, padding_end, dilations);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), 8, 447, Dimension::dynamic()}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_with_output_shape_data_dyn)
{
    const PartialShape data_pshape{PartialShape::dynamic()}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{1, 16, 2, 3, 3};       // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(
        gcbd->get_output_partial_shape(0).same_scheme(PartialShape{Dimension::dynamic(), 2, 3, 3}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_data_dyn)
{
    const PartialShape data_pshape{PartialShape::dynamic()}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{4, 5, 2, 3, 3};        // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), 8, Dimension::dynamic(), Dimension::dynamic()}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_with_output_shape_filters_dyn)
{
    const PartialShape data_pshape{
        1, 16, Dimension::dynamic(), Dimension::dynamic()}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{PartialShape::dynamic()};
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{2}, {3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(
        gcbd->get_output_partial_shape(0).same_scheme(PartialShape{1, Dimension::dynamic(), 3, 3}));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_filters_dyn)
{
    const PartialShape data_pshape{1, 8, 224, 224}; // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{PartialShape::dynamic()};
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{4}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{1, Dimension::dynamic(), Dimension::dynamic(), Dimension::dynamic()}));
}

TEST(type_prop,
     group_convolution_backprop_data_shape_infer_with_output_shape_as_const_data_and_filters_dyn)
{
    const PartialShape data_pshape{PartialShape::dynamic()};
    const PartialShape filters_pshape{PartialShape::dynamic()};
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = op::Constant::create(element::i64, Shape{3}, {3, 3, 3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_static());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().same_scheme(Rank{5}));
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(
        PartialShape{Dimension::dynamic(), Dimension::dynamic(), 3, 3, 3}));
}

TEST(type_prop,
     group_convolution_backprop_data_shape_infer_with_output_shape_as_param_data_and_filters_dyn)
{
    const PartialShape data_pshape{PartialShape::dynamic()};
    const PartialShape filters_pshape{PartialShape::dynamic()};
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto output_shape = make_shared<op::Parameter>(element::i64, Shape{3});
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(PartialShape::dynamic()));
}

TEST(type_prop, group_convolution_backprop_data_shape_infer_data_and_filters_dyn)
{
    const PartialShape data_pshape{PartialShape::dynamic()};
    const PartialShape filters_pshape{PartialShape::dynamic()};
    const element::Type_t et = element::f32;

    auto data = make_shared<op::Parameter>(et, data_pshape);
    auto filters = make_shared<op::Parameter>(et, filters_pshape);
    auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
        data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});

    ASSERT_TRUE(gcbd->get_output_partial_shape(0).rank().is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).is_dynamic());
    ASSERT_TRUE(gcbd->get_output_partial_shape(0).same_scheme(PartialShape::dynamic()));
}

TEST(type_prop, group_convolution_backprop_data_invalid_et_inputs)
{
    try
    {
        const PartialShape data_pshape{1, 16, 6, 6};      // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{2, 8, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        const element::Type_t data_et = element::f16;
        const element::Type_t filters_et = element::f32;

        auto data = make_shared<op::Parameter>(data_et, data_pshape);
        auto filters = make_shared<op::Parameter>(filters_et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});
        // data and filters should be of same element type
        FAIL() << "Incompatible element types not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Element types for data batch and filters do not match"));
    }
    catch (...)
    {
        FAIL() << "Element types validation check of inputs failed for unexpected reason";
    }

    try
    {
        const PartialShape data_pshape{1, 16, 6, 6};      // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{2, 8, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        const element::Type_t inputs_et = element::u32;

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});
        // data and filters must be of float point element type
        FAIL() << "Integral element type of inputs not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Element type of inputs must be float point"));
    }
    catch (...)
    {
        FAIL() << "Float element types of data batch and filters validation check failed for "
                  "unexpected reason.";
    }

    try
    {
        const PartialShape data_pshape{1, 16, 5, 5};       // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{1, 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        const element::Type_t inputs_et = element::f32;

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto output_shape = op::Constant::create(inputs_et, Shape{2}, {3, 3});
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);
        // output shape input element type must be of integer type
        FAIL() << "Incompatible element types not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Element type for output shape should be of integer type");
    }
    catch (...)
    {
        FAIL() << "Element types validation check of inputs failed for unexpected reason";
    }
}

TEST(type_prop, group_convolution_backprop_data_invalid_input_ranks)
{
    // data partial shape provided is rank 4 (Conv2D)
    // filter partial shape provided is rank 6 (Conv3D)
    try
    {
        const PartialShape data_pshape{1, 16, 6, 6};         // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{2, 8, 2, 3, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW, kD]

        const element::Type_t inputs_et = element::f32;

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});
        // data and filters have incompatible ranks
        FAIL() << "Incompatible input ranks not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Shapes for data batch and filters do not match."));
    }
    catch (...)
    {
        FAIL() << "Rank validation check of inputs failed for unexpected reason";
    }

    // data partial shape provided is rank 5 (Conv3D)
    // filter partial shape provided is rank 5 (Conv2D)
    try
    {
        const PartialShape data_pshape{1, 16, 6, 6, 6};   // [N, C_IN * GROUPS, H, W, D]
        const PartialShape filters_pshape{2, 8, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        const element::Type_t inputs_et = element::f32;

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, Strides{}, CoordinateDiff{}, CoordinateDiff{}, Strides{});
        // data and weight have incompatible ranks
        FAIL() << "Incompatible input ranks not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Shapes for data batch and filters do not match."));
    }
    catch (...)
    {
        FAIL() << "Rank validation check of inputs failed for unexpected reason";
    }

    try
    {
        const PartialShape data_pshape{1, 16, 5, 5};       // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{1, 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        const element::Type_t inputs_et = element::f32;

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto output_shape = op::Constant::create(element::i64, Shape{2, 1}, {3, 3});
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);
        // Output shape optional input must be of rank 1
        FAIL() << "Incompatible output shape input rank not detected.";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Spatial shape of output input must be of rank 1"));
    }
    catch (...)
    {
        FAIL() << "Rank validation check of inputs failed for unexpected reason";
    }
}

TEST(type_prop, group_convolution_backprop_data_invalid_input_channel_dims)
{
    Strides strides{1, 1};
    Strides dilations{1, 1};
    CoordinateDiff padding{2, 2};
    const element::Type_t inputs_et = element::f32;

    try
    {
        const PartialShape data_pshape{1, 16, 5, 5};         // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{21, 16, 20, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, padding, padding, dilations);
        // data batch shape does not have correct dimension C_IN * GROUPS
        FAIL() << "Incompatibile input shapes not detected.";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Number of data channels not a multiple of group size."));
    }
    catch (...)
    {
        FAIL() << "Input shapes validation check failed for unexpected reason.";
    }

    try
    {
        const PartialShape data_pshape{1, 16, 5, 5};        // [N, C_IN * GROUPS, H, W]
        const PartialShape filters_pshape{4, 16, 20, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]

        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, padding, padding, dilations);
        // filter shape specifies GROUPS = 4 and C_IN = 16, while data batch shape specifies
        // dimension C_IN * GROUPS = 16
        FAIL() << "Incompatibile input shapes not detected.";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             std::string("Data second dimension has incompatible value "
                                         "with number of input channels."));
    }
    catch (...)
    {
        FAIL() << "Input shapes validation check failed for unexpected reason.";
    }
}

TEST(type_prop, group_convolution_backprop_data_invalid_output_shape_spatial_dims)
{
    const PartialShape data_pshape{1, 16, 5, 5};      // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_shape{1, 16, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t inputs_et = element::f32;

    try
    {
        auto data = make_shared<op::Parameter>(inputs_et, data_pshape);
        auto filters = make_shared<op::Parameter>(inputs_et, filters_shape);
        auto output_shape = op::Constant::create(element::i64, Shape{3}, {3, 3, 3});
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, output_shape, Strides{}, Strides{}, op::PadType::SAME_UPPER);
        // output_shape has invalid spatials dimensions (should be 2)
        FAIL() << "Incompatible output shape optional input not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(
            error.what(),
            std::string("Output shape should be specified only and for all spatial dimensions."));
    }
    catch (...)
    {
        FAIL() << "Output shape validation check failed for unexpected reason.";
    }
}

TEST(type_prop, group_convolution_backprop_data_invalid_conv_param_spatial_dims)
{
    const PartialShape data_pshape{1, 16, 6, 6};      // [N, C_IN * GROUPS, H, W]
    const PartialShape filters_pshape{2, 8, 2, 3, 3}; // [GROUPS, C_IN, C_OUT, kH, kW]
    const element::Type_t et = element::f32;

    // invalid strides spatial dimensions
    try
    {
        Strides strides{1, 1, 1};
        Strides dilations{1, 1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0, 0};

        auto data = make_shared<op::Parameter>(et, data_pshape);
        auto filters = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations);
        FAIL() << "Invalid strides spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Strides should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Strides spatial dimensions validation check failed for unexpected reason";
    }
    try
    {
        Strides strides{1};
        Strides dilations{1, 1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0, 0};

        auto data = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto filters = make_shared<op::Parameter>(et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations);
        FAIL() << "Invalid strides spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Strides should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Strides spatial dimensions validation check failed for unexpected reason";
    }

    // invalid dilations spatial dimensions
    try
    {
        Strides strides{1, 1};
        Strides dilations{1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0, 0};

        auto data = make_shared<op::Parameter>(et, data_pshape);
        auto filters = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations);
        FAIL() << "Invalid dilations spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Dilations should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Dilations spatial dimensions validation check failed for unexpected reason";
    }
    try
    {
        Strides strides{1, 1};
        Strides dilations{1, 1, 1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0, 0};

        auto data = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto filters = make_shared<op::Parameter>(et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations);
        FAIL() << "Invalid dilations spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Dilations should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Dilations spatial dimensions validation check failed for unexpected reason";
    }

    // invalid padding spatial dimensions
    try
    {
        Strides strides{1, 1};
        Strides dilations{1, 1};
        CoordinateDiff pads_begin{0, 0, 0};
        CoordinateDiff pads_end{0, 0};

        auto data = make_shared<op::Parameter>(et, data_pshape);
        auto filters = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations);
        FAIL() << "Invalid padding spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Pads should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Padding spatial dimensions validation check failed for unexpected reason";
    }
    try
    {
        Strides strides{1, 1};
        Strides dilations{1, 1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0};

        auto data = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto filters = make_shared<op::Parameter>(et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations);
        FAIL() << "Invalid padding spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Pads should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Padding spatial dimensions validation check failed for unexpected reason";
    }

    // invalid output padding spatial dimensions
    try
    {
        Strides strides{1, 1};
        Strides dilations{1, 1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0, 0};
        CoordinateDiff output_padding{0, 0, 0};
        op::PadType auto_pad = op::PadType::EXPLICIT;

        auto data = make_shared<op::Parameter>(et, data_pshape);
        auto filters = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations, auto_pad, output_padding);
        FAIL() << "Invalid output padding spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Output padding should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Output padding spatial dimensions validation check failed for unexpected reason";
    }
    try
    {
        Strides strides{1, 1};
        Strides dilations{1, 1};
        CoordinateDiff pads_begin{0, 0};
        CoordinateDiff pads_end{0, 0};
        CoordinateDiff output_padding{0};
        op::PadType auto_pad = op::PadType::EXPLICIT;

        auto data = make_shared<op::Parameter>(et, PartialShape::dynamic());
        auto filters = make_shared<op::Parameter>(et, filters_pshape);
        auto gcbd = make_shared<op::v1::GroupConvolutionBackpropData>(
            data, filters, strides, pads_begin, pads_end, dilations, auto_pad, output_padding);
        FAIL() << "Invalid output padding spatial dimensions not detected";
    }
    catch (const NodeValidationFailure& error)
    {
        EXPECT_HAS_SUBSTRING(error.what(),
                             "Output padding should be defined for all and only spatial features.");
    }
    catch (...)
    {
        FAIL() << "Output padding spatial dimensions validation check failed for unexpected reason";
    }
}
