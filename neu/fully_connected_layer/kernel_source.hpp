#ifndef NEU_FULLY_CONNECTED_LAYER_KERNEL_SOURCE_HPP
#define NEU_FULLY_CONNECTED_LAYER_KERNEL_SOURCE_HPP
//20151023
#include <boost/compute/utility/source.hpp>
namespace neu {
	constexpr char multiply_kernel_source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
		__kernel void multiply(
			const __global float* input, __global float* output,
			const __global float* weight, const __global float* bias,
			const int input_dim, const int output_dim)
		{
			const int b = get_global_id(1);
			const int o = get_global_id(0);

			float sum = bias[o];
			for(int i = 0; i < input_dim; ++i) {
				sum += weight[i+input_dim*o]*input[i+input_dim*b];
			}
			output[o+output_dim*b] = sum;
		}
	);
	constexpr char multiply_back_kernel_source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
		__kernel void multiply_back(
			__global float* input, const __global float* output,
			const __global float* weight,
			const int input_dim, const int output_dim)
		{
			const int b = get_global_id(1);
			const int i = get_global_id(0);

			float sum = 0.0;
			for(int o = 0; o < output_dim; ++o) {
				sum += weight[i+input_dim*o]*output[o+output_dim*b];
			}
			input[i+input_dim*b] = sum;
		}
	);
	constexpr char calc_del_weight_kernel_source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
		__kernel void calc_del_weight(
			const __global float* input, const __global float* delta,
			__global float* del_weight, __global float* del_bias,
			const int input_dim, const int output_dim, const int batch_size)
		{
			const int gr = get_global_id(1);
			const int gc = get_global_id(0);

			float weight_sum = 0.0;
			float bias_sum = 0.0;
			for(int b = 0; b < batch_size; ++b) {
				weight_sum += delta[gr+output_dim*b]*input[gc+input_dim*b];
				bias_sum += delta[gr+output_dim*b];
			}
			del_weight[gc+input_dim*gr] = weight_sum/batch_size;
			del_bias[gr] = bias_sum/batch_size;
		}
	);
}// namespace neu

#endif //NEU_FULLY_CONNECTED_LAYER_KERNEL_SOURCE_HPP