#ifndef NEU_ACTIVATION_FUNC_SIGMOID_LOSS_HPP
#define NEU_ACTIVATION_FUNC_SIGMOID_LOSS_HPP
//20150528
#include <neu/assert.hpp>
#include <neu/validation.hpp>
#include <neu/range/traits.hpp>
#include <neu/range/algorithm.hpp>
#include <neu/activation_func/derivative.hpp>
#include <neu/activation_func/derivative_for_loss.hpp>
namespace neu {
	BOOST_COMPUTE_FUNCTION(float, sigmoid_loss_kernel, (float x), {
		return 1.f/(1.f+exp(-x));
	});
	class sigmoid_loss {
	public:
		template<typename InputRange, typename OutputRange>
		decltype(auto) operator()(InputRange const& input, OutputRange const& output,
				boost::compute::command_queue& queue
					=boost::compute::system::default_queue()) {
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(input, queue));
			range::transform(input, output, sigmoid_loss_kernel, queue);
			NEU_ASSERT_FOR_HEAVY_CALCULATION(is_all_of_finite(output, queue));
		}
	};
	template<>
	class derivative<sigmoid_loss> : public derivative_for_loss {};
}// namespace neu

#endif //NEU_ACTIVATION_FUNC_SIGMOID_LOSS_HPP
