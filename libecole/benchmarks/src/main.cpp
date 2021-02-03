#include <exception>
#include <iostream>
#include <iterator>
#include <optional>
#include <type_traits>
#include <vector>

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include "ecole/instance/capacitated-facility-location.hpp"
#include "ecole/instance/combinatorial-auction.hpp"
#include "ecole/instance/independent-set.hpp"
#include "ecole/instance/set-cover.hpp"
#include "ecole/random.hpp"

#include "bench-branching.hpp"
#include "benchmark.hpp"

using namespace ecole::benchmark;
using namespace ecole::instance;

/** Concatenate vectors. */
template <typename... T> auto concat(std::vector<T>... vecs) {
	using U = std::common_type_t<T...>;
	auto result = std::vector<U>{};
	result.reserve((vecs.size() + ...));
	auto inserter = std::back_inserter(result);
	((std::move(vecs.begin(), vecs.end(), inserter)), ...);
	return result;
}

/** Wrap an instance generator to a generator that also limits the number of nodes. */
template <typename Generator> auto make_generator(Generator generator, std::size_t n_nodes) {
	auto result = [generator = std::move(generator), n_nodes]() mutable {
		auto model = generator.next();
		model.disable_presolve();
		model.disable_cuts();
		model.set_param("limits/totalnodes", n_nodes);
		return model;
	};
	return result;
}

/** The generators used to benchmark branching dynamics. */
auto benchmark_branching(std::size_t n_instances, std::size_t n_nodes) {
	// Alias for the functions and parameters used.
	auto bench = [n_instances, n_nodes](auto gen, Tags tags) {
		return benchmark_branching(make_generator(std::move(gen), n_nodes), n_instances, std::move(tags));
	};

	using GraphType = typename ecole::instance::IndependentSetGenerator::Parameters::GraphType;
	return concat(
		bench(SetCoverGenerator{{500, 1000}}, {"SetCover", "Easy"}),                                         // NOLINT
		bench(SetCoverGenerator{{1000, 1000}}, {"SetCover", "Medium"}),                                      // NOLINT
		bench(SetCoverGenerator{{2000, 1000}}, {"SetCover", "Hard"}),                                        // NOLINT
		bench(CombinatorialAuctionGenerator{{100, 500}}, {"CombinatorialAuction", "Easy"}),                  // NOLINT
		bench(CombinatorialAuctionGenerator{{200, 1000}}, {"CombinatorialAuction", "Medium"}),               // NOLINT
		bench(CombinatorialAuctionGenerator{{300, 1500}}, {"CombinatorialAuction", "Hard"}),                 // NOLINT
		bench(CapacitatedFacilityLocationGenerator{{100, 100}}, {"CapacitatedFacilityLocation", "Easy"}),    // NOLINT
		bench(CapacitatedFacilityLocationGenerator{{200, 100}}, {"CapacitatedFacilityLocation", "Medium"}),  // NOLINT
		bench(CapacitatedFacilityLocationGenerator{{400, 100}}, {"CapacitatedFacilityLocation", "Hard"}),    // NOLINT
		bench(IndependentSetGenerator{{500, GraphType::erdos_renyi}}, {"IndependentSet", "Easy"}),           // NOLINT
		bench(IndependentSetGenerator{{1000, GraphType::erdos_renyi}}, {"IndependentSet", "Medium"}),        // NOLINT
		bench(IndependentSetGenerator{{1500, GraphType::erdos_renyi}}, {"IndependentSet", "Hard"}));         // NOLINT
}

template <typename T> void show(T);

int main(int argc, char** argv) {
	try {

		auto app = CLI::App{};
		app.failure_message(CLI::FailureMessage::help);
		auto n_instances = std::size_t{10};  // NOLINT
		app.add_option(
			"--intances-per-generator,--ipg", n_instances, "Number of instances generated by each instance generator");
		auto n_nodes = std::size_t{100};  // NOLINT
		app.add_option("--node-limit,--nl", n_nodes, "Limit the number of nodes in each run");
		auto seed = std::optional<ecole::Seed>{};
		app.add_option("--seed,-s", seed, "Global Ecole random seed");
		CLI11_PARSE(app, argc, argv);

		if (seed.has_value()) {
			ecole::seed(seed.value());
		}

		auto vec = benchmark_branching(n_instances, n_nodes);
		auto json = nlohmann::json{};
		json = vec;
		std::cout << json << '\n';

	} catch (std::exception const& e) {
		std::cerr << "An error occured: " << e.what() << '\n';
	}
}
