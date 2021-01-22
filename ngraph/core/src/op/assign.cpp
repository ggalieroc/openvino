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

#include "ngraph/op/assign.hpp"
#include "itt.hpp"
#include "ngraph/op/read_value.hpp"
#include "ngraph/ops.hpp"

using namespace std;
using namespace ngraph;

NGRAPH_RTTI_DEFINITION(op::v3::Assign, "Assign", 3, op::Sink);

op::v3::Assign::Assign(const Output<Node>& new_value, const std::string& variable_id)
    : Sink({new_value})
    , m_variable_id(variable_id)
{
    constructor_validate_and_infer_types();
}

void op::v3::Assign::validate_and_infer_types()
{
    NGRAPH_OP_SCOPE(v3_Assign_validate_and_infer_types);
    auto value = input_value(0);
    auto arg_t = get_input_element_type(0);
    auto input_pshape = get_input_partial_shape(0);
    auto output_pshape = PartialShape{input_pshape};

    NODE_VALIDATION_CHECK(
        this, m_variable_id != "", "Variable identifier attribute may not be an empty string.");

    if (!m_variable)
    {
        NodeVector start_nodes;
        for (const auto& input : inputs())
        {
            start_nodes.push_back(input.get_source_output().get_node_shared_ptr());
        }
        auto nodes = topological_sort(start_nodes);
        for (const auto& node : nodes)
        {
            if (auto read_value = as_type_ptr<op::v3::ReadValue>(node))
            {
                if (read_value->get_variable_id() == m_variable_id)
                    m_variable = read_value->get_variable();
            }
        }
        NODE_VALIDATION_CHECK(
            this, m_variable != nullptr, "Can't find variable with id = ", m_variable_id);
    }

    auto variable_info = m_variable->get_info();
    NODE_VALIDATION_CHECK(this,
                          m_variable_id == variable_info.variable_id,
                          "Variables identifiers are inconsistent. Got: ",
                          m_variable_id,
                          " and ",
                          variable_info.variable_id);

    NODE_VALIDATION_CHECK(this,
                          arg_t == variable_info.data_type,
                          "Variables types are inconsistent. Got: ",
                          arg_t,
                          " and ",
                          variable_info.data_type);

    NODE_VALIDATION_CHECK(this,
                          PartialShape::merge_into(output_pshape, variable_info.data_shape),
                          "Variables output shapes are inconsistent. Got: ",
                          input_pshape,
                          " and ",
                          variable_info.data_shape);

    set_output_type(0, arg_t, output_pshape);
}

shared_ptr<Node> op::v3::Assign::clone_with_new_inputs(const OutputVector& new_args) const
{
    NGRAPH_OP_SCOPE(v3_Assign_clone_with_new_inputs);
    check_new_args_count(this, new_args);
    return make_shared<Assign>(new_args.at(0), m_variable_id);
}

bool op::v3::Assign::visit_attributes(AttributeVisitor& visitor)
{
    NGRAPH_OP_SCOPE(v3_Assign_visit_attributes);
    visitor.on_attribute("variable_id", m_variable_id);
    return true;
}
