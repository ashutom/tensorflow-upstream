/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/framework/metrics.h"

#include "absl/strings/str_cat.h"
#include "tensorflow/core/lib/monitoring/counter.h"
#include "tensorflow/core/lib/monitoring/gauge.h"
#include "tensorflow/core/lib/monitoring/sampler.h"

namespace tensorflow {
namespace metrics {
namespace {

auto* graph_runs = monitoring::Counter<0>::New(
    "/tensorflow/core/graph_runs",
    "The number of graph executions used to collect "
    "/tensorflow/core/graph_run_time_usecs");

auto* graph_run_time_usecs = monitoring::Counter<0>::New(
    "/tensorflow/core/graph_run_time_usecs",
    "The total time spent on executing graphs in microseconds.");

auto* graph_optimization_usecs =
    monitoring::Counter<2>::New("/tensorflow/core/graph_optimization_usecs",
                                "The total time spent running each graph "
                                "optimization pass in microseconds.",
                                "kind", "name");

auto* graph_run_time_usecs_histogram = monitoring::Sampler<0>::New(
    {"/tensorflow/core/graph_run_time_usecs_histogram",
     "The wall-clock time spent on executing graphs in microseconds."},
    // Power of 2 with bucket count 20 (> 17 minutes)
    {monitoring::Buckets::Exponential(1000, 2, 20)});

auto* graph_pending_queue_length_histogram = monitoring::Sampler<0>::New(
    {"/tensorflow/core/graph_pending_queue_length_histogram",
     "The number of pending (ready but not running) tasks in graph executor."},
    // Power of 1.5 with bucket count 30 (> 191k)
    {monitoring::Buckets::Exponential(1, 1.5, 30)});

auto* graph_run_input_tensor_bytes = monitoring::Sampler<0>::New(
    {"/tensorflow/core/graph_run_input_tensor_bytes",
     "The size of input tensors in bytes."},
    // Power of 2 with bucket count 14 (256MB)
    {monitoring::Buckets::Exponential(1, 4, 14)});

auto* graph_run_output_tensor_bytes = monitoring::Sampler<0>::New(
    {"/tensorflow/core/graph_run_output_tensor_bytes",
     "The size of output tensors in bytes."},
    // Power of 2 with bucket count 14 (256MB)
    {monitoring::Buckets::Exponential(1, 4, 14)});

auto* graph_unused_outputs = monitoring::Counter<1>::New(
    "/tensorflow/core/graph_unused_outputs",
    "The number of unused outputs for ops of a given type.", "name");

auto* tf_data_autotune_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/autotune", "tf.data autotuning", "name");

auto* tf_data_bytes_consumed_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/bytes_consumed",
    "The number of bytes consumed by a tf.data Dataset.", "name");

auto* tf_data_bytes_produced_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/bytes_produced",
    "The number of bytes produced by a tf.data Dataset.", "name");

auto* tf_data_bytes_read_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/bytes_read",
    "The number of bytes read by tf.data Dataset sources.", "name");

auto* tf_data_bytes_fetched_counter = monitoring::Counter<0>::New(
    "/tensorflow/data/bytes_fetched",
    "The number of bytes fetched from tf.data Dataset iterator.");

auto* tf_data_elements_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/elements", "tf.data elements", "name");

auto* tf_data_experiment_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/experiment",
    "The number of times tf.data experiment is applied to input pipelines.",
    "name");

auto* tf_data_fingerprint_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/fingerprint", "tf.data fingerprint", "name");

auto* tf_data_get_next_duration_usecs_histogram = monitoring::Sampler<0>::New(
    {"/tensorflow/data/getnext_duration",
     "Microseconds spent fetching an element from tf.data iterator."},
    // Power of 2 with bucket count 10 (1024 microseconds) and 1 second.
    {monitoring::Buckets::Explicit(
        {2., 4., 8., 16., 32., 64., 128., 256., 512., 1024., 1e6})});

auto* tf_data_iterator_busy_counter =
    monitoring::Counter<0>::New("/tensorflow/data/iterator_busy",
                                "The time (in microseconds) during which a "
                                "tf.data iterator was busy processing at "
                                "least one `GetNext()` request.");

auto* tf_data_iterator_lifetime_counter = monitoring::Counter<0>::New(
    "/tensorflow/data/iterator_lifetime",
    "The time (in microseconds) between a tf.data iterator receiving the first "
    "`GetNext()` request and responding to the last `GetNext()` request.");

auto* tf_data_optimization_counter = monitoring::Counter<1>::New(
    "/tensorflow/data/optimization", "tf.data optimization", "name");

auto* tf_data_service_workers_created_counter =
    monitoring::Counter<0>::New("/tensorflow/data/service/workers_created",
                                "Number of tf.data service workers created");

auto* tf_data_filename_counter = monitoring::Counter<2>::New(
    "/tensorflow/data/filename", "The file name read by a tf.data Dataset.",
    "name", "filename");

auto* tf_data_model_gauge =
    monitoring::Gauge<std::function<std::string()>, 1>::New(
        "/tensorflow/data/model", "tf.data autotuning model proto.", "id");

auto* parse_dense_feature_counter = monitoring::Counter<0>::New(
    "/tensorflow/data/dense_feature",
    "The number of dense features parsed by ops for parsing tf.Example.");

auto* parse_sparse_feature_counter = monitoring::Counter<0>::New(
    "/tensorflow/data/sparse_feature",
    "The number of sparse features parsed by ops for parsing tf.Example.");

auto* parse_ragged_feature_counter = monitoring::Counter<0>::New(
    "/tensorflow/data/ragged_feature",
    "The number of ragged features parsed by ops for parsing tf.Example.");

auto* build_graph_calls = monitoring::Counter<0>::New(
    "/tensorflow/core/graph_build_calls",
    "The number of times TensorFlow has created a new client graph. "
    "A client graph is a sub-graph of the full graph, induced by a set of "
    "options, including the requested feeds and fetches. It includes time "
    "spent optimizing the graph with Grappler, and time spent pruning the "
    "sub-graph.");

auto* build_graph_time_usecs = monitoring::Counter<0>::New(
    "/tensorflow/core/graph_build_time_usecs",
    "The amount of time TensorFlow has spent creating new client graphs in "
    "microseconds. "
    "A client graph is a sub-graph of the full graph, induced by a set of "
    "options, including the requested feeds and fetches. It includes time "
    "spent optimizing the graph with Grappler, and time spent pruning the "
    "sub-graph.");

auto* xla_compilations = monitoring::Counter<0>::New(
    "/tensorflow/core/xla_compilations",
    "The number of XLA compilations used to collect "
    "/tensorflow/core/xla_compilation_time_usecs");

auto* xla_compilation_time_usecs = monitoring::Counter<0>::New(
    "/tensorflow/core/xla_compilation_time_usecs",
    "The total time spent on compiling XLA graphs in microseconds.");

auto* xla_tpu_spmd_cores_per_replica = monitoring::Counter<1>::New(
    "/tensorflow/tpu/xla_spmd_cores_per_replica",
    "The number of cores used by XLA SPMD-replicated models.", "cores");

auto* bfc_allocator_delay =
    monitoring::Counter<0>::New("/tensorflow/core/bfc_allocator_delay",
                                "The total time spent running each graph "
                                "optimization pass in microseconds.");

auto* tpu_variable_distribution_time_usecs = monitoring::Counter<0>::New(
    "/tensorflow/tpu/variable_distribution_time",
    "Time spent sending variables from primary task to other worker tasks "
    "at the start of a call to TPUExecute.  Timer starts at RunGraph "
    "invocation and ends when TPUExecute args are ready on the current task.");

}  // namespace

void RecordTFDataAutotune(const string& name) {
  tf_data_autotune_counter->GetCell(name)->IncrementBy(1);
}

monitoring::CounterCell* GetTFDataBytesConsumedCounter(const string& name) {
  return tf_data_bytes_consumed_counter->GetCell(name);
}

monitoring::CounterCell* GetTFDataBytesProducedCounter(const string& name) {
  return tf_data_bytes_produced_counter->GetCell(name);
}

monitoring::CounterCell* GetTFDataBytesReadCounter(const string& name) {
  return tf_data_bytes_read_counter->GetCell(name);
}

monitoring::CounterCell* GetTFDataElementsCounter(const string& name) {
  return tf_data_elements_counter->GetCell(name);
}

monitoring::GaugeCell<std::function<std::string()>>* GetTFDataModelGauge(
    const string& id) {
  return tf_data_model_gauge->GetCell(id);
}

void RecordTFDataBytesFetched(int64_t num_bytes) {
  tf_data_bytes_fetched_counter->GetCell()->IncrementBy(num_bytes);
}

void RecordTFDataExperiment(const string& name) {
  tf_data_experiment_counter->GetCell(name)->IncrementBy(1);
}

void RecordTFDataFingerprint(const string& name) {
  tf_data_fingerprint_counter->GetCell(name)->IncrementBy(1);
}

void RecordTFDataGetNextDuration(uint64 duration_us) {
  static auto* tf_data_get_next_duration_cell =
      tf_data_get_next_duration_usecs_histogram->GetCell();
  tf_data_get_next_duration_cell->Add(duration_us);
}

void RecordTFDataIteratorBusy(uint64 duration_us) {
  static auto* tf_data_iterator_busy_cell =
      tf_data_iterator_busy_counter->GetCell();
  tf_data_iterator_busy_cell->IncrementBy(duration_us);
}

void RecordTFDataIteratorLifetime(uint64 duration_us) {
  static auto* tf_data_iterator_lifetime_cell =
      tf_data_iterator_lifetime_counter->GetCell();
  tf_data_iterator_lifetime_cell->IncrementBy(duration_us);
}

void RecordTFDataOptimization(const string& name, int64_t num_changes) {
  tf_data_optimization_counter->GetCell(name)->IncrementBy(num_changes);
}

void RecordTFDataServiceWorkerCreated() {
  tf_data_service_workers_created_counter->GetCell()->IncrementBy(1);
}

void RecordTFDataFilename(const string& name, const string& filename) {
  tf_data_filename_counter->GetCell(name, filename)->IncrementBy(1);
}

void RecordParseDenseFeature(int64_t num_features) {
  static auto* parse_dense_feature_counter_cell =
      parse_dense_feature_counter->GetCell();
  parse_dense_feature_counter_cell->IncrementBy(num_features);
}

void RecordParseSparseFeature(int64_t num_features) {
  static auto* parse_sparse_feature_counter_cell =
      parse_sparse_feature_counter->GetCell();
  parse_sparse_feature_counter_cell->IncrementBy(num_features);
}

void RecordParseRaggedFeature(int64_t num_features) {
  static auto* parse_ragged_feature_counter_cell =
      parse_ragged_feature_counter->GetCell();
  parse_ragged_feature_counter_cell->IncrementBy(num_features);
}

void RecordGraphInputTensors(const size_t size) {
  static auto* graph_run_input_tensor_bytes_cell =
      graph_run_input_tensor_bytes->GetCell();
  graph_run_input_tensor_bytes_cell->Add(size);
}

void RecordGraphOutputTensors(const size_t size) {
  static auto* graph_run_output_tensor_bytes_cell =
      graph_run_output_tensor_bytes->GetCell();
  graph_run_output_tensor_bytes_cell->Add(size);
}

void RecordTPUXlaSpmdCoresPerReplica(int64_t cores_per_replica) {
  xla_tpu_spmd_cores_per_replica->GetCell(absl::StrCat(cores_per_replica))
      ->IncrementBy(1);
}

void UpdateGraphExecTime(const uint64 running_time_usecs) {
  if (running_time_usecs > 0) {
    static auto* graph_runs_cell = graph_runs->GetCell();
    static auto* graph_run_time_usecs_cell = graph_run_time_usecs->GetCell();
    static auto* graph_run_time_usecs_histogram_cell =
        graph_run_time_usecs_histogram->GetCell();
    graph_runs_cell->IncrementBy(1);
    graph_run_time_usecs_cell->IncrementBy(running_time_usecs);
    graph_run_time_usecs_histogram_cell->Add(running_time_usecs);
  }
}

void UpdateGraphPendingQueueLength(uint64 len) {
  static auto* graph_pending_queue_length_cell =
      graph_pending_queue_length_histogram->GetCell();
  graph_pending_queue_length_cell->Add(len);
}

void UpdateGraphOptimizationPassTime(const string& pass_name,
                                     const uint64 running_time_usecs) {
  if (running_time_usecs > 0) {
    graph_optimization_usecs->GetCell("GraphOptimizationPass", pass_name)
        ->IncrementBy(running_time_usecs);
  }
}

void UpdateGrapplerPassTime(const string& pass_name,
                            const uint64 running_time_usecs) {
  if (running_time_usecs > 0) {
    graph_optimization_usecs->GetCell("Grappler", pass_name)
        ->IncrementBy(running_time_usecs);
  }
}

void UpdateGraphBuildTime(const uint64 running_time_usecs) {
  if (running_time_usecs > 0) {
    static auto* build_graph_calls_cell = build_graph_calls->GetCell();
    static auto* build_graph_time_usecs_cell =
        build_graph_time_usecs->GetCell();
    build_graph_calls_cell->IncrementBy(1);
    build_graph_time_usecs_cell->IncrementBy(running_time_usecs);
  }
}

void UpdateTpuVariableDistributionTime(const uint64 distribution_time_usecs) {
  if (distribution_time_usecs > 0) {
    tpu_variable_distribution_time_usecs->GetCell()->IncrementBy(
        distribution_time_usecs);
  }
}

void UpdateXlaCompilationTime(const uint64 compilation_time_usecs) {
  if (compilation_time_usecs > 0) {
    static auto* xla_compilations_cell = xla_compilations->GetCell();
    static auto* xla_compilation_time_usecs_cell =
        xla_compilation_time_usecs->GetCell();
    xla_compilations_cell->IncrementBy(1);
    xla_compilation_time_usecs_cell->IncrementBy(compilation_time_usecs);
  }
}

void UpdateBfcAllocatorDelayTime(const uint64 delay_usecs) {
  static auto* bfc_allocator_delay_cell = bfc_allocator_delay->GetCell();
  if (delay_usecs > 0) {
    bfc_allocator_delay_cell->IncrementBy(delay_usecs);
  }
}

void RecordUnusedOutput(const string& op_name) {
  graph_unused_outputs->GetCell(op_name)->IncrementBy(1);
}

}  // namespace metrics
}  // namespace tensorflow
