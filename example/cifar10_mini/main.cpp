#include <iostream>
#include <boost/timer.hpp>
#include <neu/vector_io.hpp>
#include <neu/layers_algorithm.hpp>
#include <neu/kernel.hpp>
//#include <neu/learning_rate_gen/subtract_delta_weight.hpp>
//#include <neu/learning_rate_gen/fixed_learning_rate_gen.hpp>
//#include <neu/learning_rate_gen/weight_decay.hpp>
#include <neu/learning_rate_gen/weight_decay_and_momentum.hpp>
#include <neu/activation_func/sigmoid.hpp>
#include <neu/activation_func/rectifier.hpp>
#include <neu/activation_func/identity.hpp>
#include <neu/activation_func/softmax.hpp>
#include <neu/activation_layer.hpp>
#include <neu/convolution_layer.hpp>
#include <neu/max_pooling_layer.hpp>
#include <neu/average_pooling_layer.hpp>
#include <neu/fully_connected_layer.hpp>
#include <neu/layer.hpp>
#include <neu/load_data_set/load_cifar10.hpp>
#include <neu/data_set.hpp>

int main(int argc, char** argv) {
	std::cout << "hello world" << std::endl;

	constexpr auto label_num = 10u;
	constexpr auto data_num_per_label = 10u;
	constexpr auto input_dim = 32u*32u*3u;
	constexpr auto batch_size = label_num * data_num_per_label;

	//std::random_device rd; std::mt19937 rand(rd());
	std::mt19937 rand(0); std::cout << "INFO: fixed random engine" << std::endl;

	auto data = neu::load_cifar10("../../../data/cifar-10-batches-bin/");
	for(auto& labeled : data) {
		for(auto& d : labeled) {
			std::transform(d.begin(), d.end(), d.begin(),
				[](auto e){ return (e-127.)/255.; });
		}
	}
	auto ds = neu::make_data_set(label_num, data_num_per_label, input_dim, data, rand);

	auto conv1_param = neu::convolution_layer_parameter()
		.input_width(32).input_channel_num(3).output_channel_num(32)
		.filter_width(5).stride(1).pad(2).batch_size(batch_size);
	auto pool1_param = neu::make_max_pooling_layer_parameter(conv1_param)
		.filter_width(3).stride(2).pad(1);
	auto relu1_param = neu::make_activation_layer_parameter(pool1_param);
	auto fc1_param = neu::make_fully_connected_layer_parameter(pool1_param)
		.output_dim(64);
	auto fc2_param = neu::make_fully_connected_layer_parameter(fc1_param)
		.output_dim(label_num);
	auto softmax_param = neu::make_activation_layer_parameter(fc2_param);

	auto conv1_g = [&rand, dist=std::normal_distribution<>(0.f, 0.0001f)]
		() mutable { return dist(rand); };
	auto fc12_g = [&rand, dist=std::normal_distribution<>(0.f, 0.1f)]
		() mutable { return dist(rand); };

	neu::scalar base_lr = 0.001;
	neu::scalar momentum = 0.9;
	neu::scalar weight_decay = 0.005;
	//neu::scalar momentum = 0.;
	//neu::scalar weight_decay = 0.;
	auto conv1 = neu::make_convolution_layer(conv1_param, conv1_g,
		neu::weight_decay_and_momentum(base_lr, momentum, weight_decay,
			conv1_param.weight_dim(), conv1_param.bias_dim()));
	auto pool1 = neu::make_max_pooling_layer(pool1_param);
	auto relu1 = neu::make_activation_layer(relu1_param, neu::rectifier());
	auto fc1 = neu::make_fully_connected_layer(fc1_param, fc12_g,
		neu::weight_decay_and_momentum(base_lr, momentum, weight_decay,
			fc1_param.weight_dim(), fc1_param.bias_dim()));
	auto fc2 = neu::make_fully_connected_layer(fc2_param, fc12_g,
		neu::weight_decay_and_momentum(base_lr, momentum, weight_decay,
			fc2_param.weight_dim(), fc2_param.bias_dim()));
	auto softmax = neu::make_activation_layer(softmax_param, neu::softmax(10, batch_size));

	auto layers = std::vector<neu::layer>{
		std::ref(conv1),
		pool1,
		relu1,
		std::ref(fc1),
		std::ref(fc2),
		softmax
	};
	std::ofstream error_log("error.txt");
	make_next_batch(ds);
	boost::timer timer;
	boost::timer t2;
	for(auto i = 0u; i < 5000u; ++i) {
		timer.restart();
		auto batch = ds.get_batch();
		auto input = batch.train_data;
		auto teach = batch.teach_data;
		auto make_next_batch_future = ds.async_make_next_batch();

		std::cout << "forward..." << std::endl;
		neu::layers_forward(layers, input);
		std::cout << "forward finished " << timer.elapsed() << std::endl;
		timer.restart();

		auto output = layers.back().get_next_input();
		std::ofstream outputf("output"+std::to_string(i)+".txt");
		neu::print(outputf, output, 10);

		Ensures(boost::compute::all_of(output.begin(), output.end(),
			0 <= boost::compute::lambda::_1));
		Ensures(boost::compute::all_of(output.begin(), output.end(),
			boost::compute::lambda::_1 <= 1.f));
		volatile auto output_sum = boost::compute::accumulate(output.begin(), output.end(),
			0.f, boost::compute::plus<neu::scalar>());
		Ensures(batch_size-1 < output_sum && output_sum < batch_size+1);
		neu::gpu_vector error(output.size());
		boost::compute::transform(output.begin(), output.end(),
			teach.begin(), error.begin(), boost::compute::minus<neu::scalar>());

		neu::gpu_vector squared_error(error.size());
		boost::compute::transform(error.begin(), error.end(),
			error.begin(), squared_error.begin(),
			boost::compute::multiplies<neu::scalar>());
		auto squared_error_sum = boost::compute::accumulate(
			squared_error.begin(), squared_error.end(), 0.f);
		std::cout << i << ":squared_error_sum: " << squared_error_sum << std::endl;
		error_log << i << "\t" << squared_error_sum << std::endl;
		//neu::print(error);

		std::cout << "backward..." << std::endl;
		neu::layers_backward(layers, error);
		std::cout << "backward finished" << timer.elapsed() << std::endl;
		timer.restart();

		std::cout << "update..." << std::endl;
		conv1.update();
		fc1.update();
		fc2.update();
		std::cout << "update finished" << timer.elapsed() << std::endl;
		timer.restart();

		make_next_batch_future.wait();
	}
}
