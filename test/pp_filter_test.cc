/*
 *  Copyright (c) 2012 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "./vpx_config.h"
#include "./vpx_dsp_rtcd.h"
#include "test/acm_random.h"
#include "test/clear_system_state.h"
#include "test/register_state_check.h"
#include "third_party/googletest/src/include/gtest/gtest.h"
#include "vpx/vpx_integer.h"
#include "vpx_mem/vpx_mem.h"

typedef void (*VpxPostProcDownAndAcrossMbRowFunc)(
    unsigned char *src_ptr, unsigned char *dst_ptr, int src_pixels_per_line,
    int dst_pixels_per_line, int cols, unsigned char *flimit, int size);

namespace {

class VpxPostProcDownAndAcrossMbRowTest
    : public ::testing::TestWithParam<VpxPostProcDownAndAcrossMbRowFunc> {
 public:
  virtual void TearDown() { libvpx_test::ClearSystemState(); }
};

// Test routine for the VPx post-processing function
// vpx_post_proc_down_and_across_mb_row_c.

TEST_P(VpxPostProcDownAndAcrossMbRowTest, CheckFilterOutput) {
  // Size of the underlying data block that will be filtered.
  const int block_width = 16;
  const int block_height = 16;

  // 5-tap filter needs 2 padding rows above and below the block in the input.
  const int input_width = block_width;
  const int input_height = block_height + 4;
  const int input_stride = input_width;
  const int input_size = input_width * input_height;

  // Filter extends output block by 8 samples at left and right edges.
  const int output_width = block_width + 16;
  const int output_height = block_height;
  const int output_stride = output_width;
  const int output_size = output_width * output_height;

  uint8_t *const src_image =
      reinterpret_cast<uint8_t *>(vpx_calloc(input_size, 1));
  uint8_t *const dst_image =
      reinterpret_cast<uint8_t *>(vpx_calloc(output_size, 1));

  // Pointers to top-left pixel of block in the input and output images.
  uint8_t *const src_image_ptr = src_image + (input_stride << 1);
  uint8_t *const dst_image_ptr = dst_image + 8;
  uint8_t *const flimits =
      reinterpret_cast<uint8_t *>(vpx_memalign(16, block_width));
  (void)memset(flimits, 255, block_width);

  // Initialize pixels in the input:
  //   block pixels to value 1,
  //   border pixels to value 10.
  (void)memset(src_image, 10, input_size);
  uint8_t *pixel_ptr = src_image_ptr;
  for (int i = 0; i < block_height; ++i) {
    for (int j = 0; j < block_width; ++j) {
      pixel_ptr[j] = 1;
    }
    pixel_ptr += input_stride;
  }

  // Initialize pixels in the output to 99.
  (void)memset(dst_image, 99, output_size);

  ASM_REGISTER_STATE_CHECK(GetParam()(src_image_ptr, dst_image_ptr,
                                      input_stride, output_stride, block_width,
                                      flimits, 16));

  static const uint8_t kExpectedOutput[block_height] = {
    4, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 4
  };

  pixel_ptr = dst_image_ptr;
  for (int i = 0; i < block_height; ++i) {
    for (int j = 0; j < block_width; ++j) {
      ASSERT_EQ(kExpectedOutput[i], pixel_ptr[j]);
    }
    pixel_ptr += output_stride;
  }

  vpx_free(src_image);
  vpx_free(dst_image);
  vpx_free(flimits);
};

INSTANTIATE_TEST_CASE_P(
    C, VpxPostProcDownAndAcrossMbRowTest,
    ::testing::Values(vpx_post_proc_down_and_across_mb_row_c));

#if HAVE_SSE2
INSTANTIATE_TEST_CASE_P(
    SSE2, VpxPostProcDownAndAcrossMbRowTest,
    ::testing::Values(vpx_post_proc_down_and_across_mb_row_sse2));
#endif

#if HAVE_MSA
INSTANTIATE_TEST_CASE_P(
    MSA, VpxPostProcDownAndAcrossMbRowTest,
    ::testing::Values(vpx_post_proc_down_and_across_mb_row_msa));
#endif

}  // namespace
