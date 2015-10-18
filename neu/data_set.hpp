#ifndef NEU_DATA_SET_HPP
#define NEU_DATA_SET_HPP
//20151008
#include <vector>
#include <thread>
#include <future>
#include <boost/compute/algorithm/copy.hpp>
#include <neu/basic_type.hpp>
namespace neu {
	struct batch {
		gpu_vector train_data;
		gpu_vector teach_data;
	};
	template<typename UniformRandomNumberGenerator>
	class data_set {
	public:
		data_set(std::size_t label_num, std::size_t data_num_per_label,
			std::size_t input_dim, std::vector<std::vector<cpu_vector>> const& data,
			UniformRandomNumberGenerator const& g)
			: label_num_(label_num), data_num_per_label_(data_num_per_label),
			input_dim_(input_dim), data_(data), g_(g),
			batch_{gpu_vector(label_num*data_num_per_label*input_dim), 
				gpu_vector(label_num*label_num*data_num_per_label)} {}

		decltype(auto) async_make_next_batch() {
			return std::async(std::launch::async, [this](){
				cpu_vector cpu_train_data;
				cpu_vector cpu_teach_data;
				for(auto i = 0u; i < label_num_; ++i) {
					std::shuffle(data_[i].begin(), data_[i].end(), g_);
					for(auto j = 0u; j < data_num_per_label_; ++j) {
						cpu_train_data.insert(cpu_train_data.end(),
							data_[i][j].begin(), data_[i][j].end());
					}

					cpu_vector teach(label_num_, 0.f); teach[i] = 1.f;
					for(auto j = 0u; j < data_num_per_label_; ++j) {
						cpu_teach_data.insert(cpu_teach_data.end(),
							teach.begin(), teach.end());
					}
				}
				auto train_copy_future = boost::compute::copy_async(
					cpu_train_data.begin(), cpu_train_data.end(),
					batch_.train_data.begin());
				auto teach_copy_future = boost::compute::copy_async(
					cpu_teach_data.begin(), cpu_teach_data.end(),
					batch_.teach_data.begin());
				train_copy_future.wait();
				teach_copy_future.wait();
			});
		}
		decltype(auto) get_batch() const { return (batch_); }
	private:
		std::size_t label_num_;
		std::size_t data_num_per_label_;
		std::size_t input_dim_;
		std::vector<std::vector<cpu_vector>> data_;
		UniformRandomNumberGenerator g_;
		batch batch_;
	};
	template<typename UniformRandomNumberGenerator>
	decltype(auto) make_next_batch(data_set<UniformRandomNumberGenerator>& ds) {
		ds.async_make_next_batch().wait();
	}
	template<typename UniformRandomNumberGenerator>
	decltype(auto) make_data_set(std::size_t label_num, std::size_t data_num_per_label,
			std::size_t input_dim, std::vector<std::vector<cpu_vector>> const& data,
			UniformRandomNumberGenerator const& g) {
		return data_set<UniformRandomNumberGenerator>(label_num, data_num_per_label,
			input_dim, data, g);
	}

}// namespace neu

#endif //NEU_DATA_SET_HPP