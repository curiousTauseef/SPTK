// ----------------------------------------------------------------- //
//             The Speech Signal Processing Toolkit (SPTK)           //
//             developed by SPTK Working Group                       //
//             http://sp-tk.sourceforge.net/                         //
// ----------------------------------------------------------------- //
//                                                                   //
//  Copyright (c) 1984-2007  Tokyo Institute of Technology           //
//                           Interdisciplinary Graduate School of    //
//                           Science and Engineering                 //
//                                                                   //
//                1996-2017  Nagoya Institute of Technology          //
//                           Department of Computer Science          //
//                                                                   //
// All rights reserved.                                              //
//                                                                   //
// Redistribution and use in source and binary forms, with or        //
// without modification, are permitted provided that the following   //
// conditions are met:                                               //
//                                                                   //
// - Redistributions of source code must retain the above copyright  //
//   notice, this list of conditions and the following disclaimer.   //
// - Redistributions in binary form must reproduce the above         //
//   copyright notice, this list of conditions and the following     //
//   disclaimer in the documentation and/or other materials provided //
//   with the distribution.                                          //
// - Neither the name of the SPTK working group nor the names of its //
//   contributors may be used to endorse or promote products derived //
//   from this software without specific prior written permission.   //
//                                                                   //
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            //
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       //
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          //
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          //
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS //
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          //
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   //
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     //
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON //
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   //
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    //
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           //
// POSSIBILITY OF SUCH DAMAGE.                                       //
// ----------------------------------------------------------------- //

#include <getopt.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "SPTK/math/fast_fourier_transform.h"
#include "SPTK/utils/sptk_utils.h"

namespace {

enum OutputFormats {
  kOutputRealAndImaginaryParts = 0,
  kOutputRealPart,
  kOutputImaginaryPart,
  kOutputAmplitude,
  kOutputPower,
  kNumOutputFormats
};

const int kDefaultFftLength(256);
const OutputFormats kDefaultOutputFormat(kOutputRealAndImaginaryParts);

void PrintUsage(std::ostream* stream) {
  // clang-format off
  *stream << std::endl;
  *stream << " fft - FFT for complex sequence" << std::endl;
  *stream << std::endl;
  *stream << "  usage:" << std::endl;
  *stream << "       fft [ options ] [ infile ] > stdout" << std::endl;
  *stream << "  options:" << std::endl;
  *stream << "       -l l  : FFT length                     (   int)[" << std::setw(5) << std::right << kDefaultFftLength    << "][ 4 <= l <=   ]" << std::endl;  // NOLINT
  *stream << "       -m m  : order of sequence              (   int)[" << std::setw(5) << std::right << "l-1"                << "][ 0 <= m <  l ]" << std::endl;  // NOLINT
  *stream << "       -o o  : output format                  (   int)[" << std::setw(5) << std::right << kDefaultOutputFormat << "][ 0 <= o <= 4 ]" << std::endl;  // NOLINT
  *stream << "                 0 (real and imaginary parts)" << std::endl;
  *stream << "                 1 (real part)" << std::endl;
  *stream << "                 2 (imaginary part)" << std::endl;
  *stream << "                 3 (amplitude)" << std::endl;
  *stream << "                 4 (power)" << std::endl;
  *stream << "       -h    : print this message" << std::endl;
  *stream << "  infile:" << std::endl;
  *stream << "       data sequence                          (double)[stdin]" << std::endl;  // NOLINT
  *stream << "  stdout:" << std::endl;
  *stream << "       FFT sequence                           (double)" << std::endl;  // NOLINT
  *stream << "  notice:" << std::endl;
  *stream << "       value of l must be a power of 2" << std::endl;
  *stream << std::endl;
  *stream << " SPTK: version " << sptk::kVersion << std::endl;
  *stream << std::endl;
  // clang-format on
}

}  // namespace

int main(int argc, char* argv[]) {
  int fft_length(kDefaultFftLength);
  int num_order(kDefaultFftLength - 1);
  bool is_num_order_specified(false);
  OutputFormats output_format(kDefaultOutputFormat);

  for (;;) {
    const int option_char(getopt_long(argc, argv, "l:m:o:h", NULL, NULL));
    if (-1 == option_char) break;

    switch (option_char) {
      case 'l': {
        if (!sptk::ConvertStringToInteger(optarg, &fft_length)) {
          std::ostringstream error_message;
          error_message << "The argument for the -l option must be an integer";
          sptk::PrintErrorMessage("fft", error_message);
          return 1;
        }
        break;
      }
      case 'm': {
        if (!sptk::ConvertStringToInteger(optarg, &num_order) ||
            num_order < 0) {
          std::ostringstream error_message;
          error_message << "The argument for the -m option must be a "
                        << "non-negative integer";
          sptk::PrintErrorMessage("fft", error_message);
          return 1;
        }
        is_num_order_specified = true;
        break;
      }
      case 'o': {
        const int min(0);
        const int max(static_cast<int>(kNumOutputFormats) - 1);
        int tmp;
        if (!sptk::ConvertStringToInteger(optarg, &tmp) ||
            !sptk::IsInRange(tmp, min, max)) {
          std::ostringstream error_message;
          error_message << "The argument for the -o option must be an integer "
                        << "in the range of " << min << " to " << max;
          sptk::PrintErrorMessage("fft", error_message);
          return 1;
        }
        output_format = static_cast<OutputFormats>(tmp);
        break;
      }
      case 'h': {
        PrintUsage(&std::cout);
        return 0;
      }
      default: {
        PrintUsage(&std::cerr);
        return 1;
      }
    }
  }

  // check order
  if (!is_num_order_specified) {
    num_order = fft_length - 1;
  } else if (fft_length <= num_order) {
    std::ostringstream error_message;
    error_message << "The order of data sequence " << num_order
                  << " must be less than FFT length " << fft_length;
    sptk::PrintErrorMessage("fft", error_message);
    return 1;
  }

  // get input file
  const char* input_file((optind < argc) ? argv[argc - 1] : NULL);

  // open stream
  std::ifstream ifs;
  ifs.open(input_file, std::ios::in | std::ios::binary);
  if (ifs.fail() && NULL != input_file) {
    std::ostringstream error_message;
    error_message << "Cannot open file " << input_file;
    sptk::PrintErrorMessage("fft", error_message);
    return 1;
  }
  std::istream& input_stream(ifs.fail() ? std::cin : ifs);

  // prepare for fast Fourier transform
  sptk::FastFourierTransform fft(num_order, fft_length);
  if (!fft.IsValid()) {
    std::ostringstream error_message;
    error_message << "FFT length must be a power of 2 and greater than 2";
    sptk::PrintErrorMessage("fft", error_message);
    return 1;
  }

  const int length(num_order + 1);
  std::vector<double> input_x(length);
  std::vector<double> input_y(length);
  std::vector<double> output_x(fft_length);
  std::vector<double> output_y(fft_length);

  while (sptk::ReadStream(true, 0, 0, length, &input_x, &input_stream) &&
         sptk::ReadStream(true, 0, 0, length, &input_y, &input_stream)) {
    if (!fft.Run(input_x, input_y, &output_x, &output_y)) {
      std::ostringstream error_message;
      error_message << "Failed to run fast Fourier transform";
      sptk::PrintErrorMessage("fft", error_message);
      return 1;
    }

    if (kOutputAmplitude == output_format) {
      for (int i(0); i < fft_length; ++i) {
        output_x[i] =
            std::sqrt(output_x[i] * output_x[i] + output_y[i] * output_y[i]);
      }
    } else if (kOutputPower == output_format) {
      for (int i(0); i < fft_length; ++i) {
        output_x[i] = output_x[i] * output_x[i] + output_y[i] * output_y[i];
      }
    }

    if (kOutputImaginaryPart != output_format &&
        !sptk::WriteStream(0, fft_length, output_x, &std::cout)) {
      std::ostringstream error_message;
      error_message << "Failed to write output sequence";
      sptk::PrintErrorMessage("fft", error_message);
      return 1;
    }

    if ((kOutputRealAndImaginaryParts == output_format ||
         kOutputImaginaryPart == output_format) &&
        !sptk::WriteStream(0, fft_length, output_y, &std::cout)) {
      std::ostringstream error_message;
      error_message << "Failed to write imaginary parts";
      sptk::PrintErrorMessage("fft", error_message);
      return 1;
    }
  }

  return 0;
}
