#ifndef NEU_ACTIVATION_FUNC_IDENTITY_HPP
#define NEU_ACTIVATION_FUNC_IDENTITY_HPP
//20150528
#include <neu/basic_type.hpp>
#include <neu/activation_func/differential.hpp>
namespace neu {
	class identity {
	public:
		template<typename T>
		decltype(auto) operator()(T&& x) const {
			return std::forward<T>(x);
		}
	};
	template<>
	class differential<identity> {
	public:
		decltype(auto) operator()(neu::gpu_vector x) const {
			boost::compute::fill(x.begin(), x.end(), 1.);
			boost::compute::system::default_queue().finish();
			return x;
		}
	};
}// namespace neu

#endif //NEU_ACTIVATION_FUNC_IDENTITY_HPP