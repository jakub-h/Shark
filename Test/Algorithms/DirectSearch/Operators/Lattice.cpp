#define BOOST_TEST_MODULE DirectSearch_Operators_Lattice

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include <shark/Algorithms/DirectSearch/Operators/Lattice.h>
#include <shark/ObjectiveFunctions/Benchmarks/Benchmarks.h>
#include <shark/Core/Random.h>
#include <shark/LinAlg/Metrics.h>

#include <iostream>

using namespace shark;

BOOST_AUTO_TEST_SUITE (Algorithms_DirectSearch_Operators_Lattice)

BOOST_AUTO_TEST_CASE(unitVectors_correct){
	for(std::size_t n = 2; n < 6; ++n){
		for(std::size_t s = 3; s < 10; ++s){
			RealMatrix weights = weightLattice(n, s);
			RealMatrix refvecs = unitVectorsOnLattice(n, s);
			BOOST_CHECK_EQUAL(weights.size1(), refvecs.size1());
			BOOST_CHECK_EQUAL(weights.size2(), refvecs.size2());
			// All vectors must be of length 1 since they are unit vectors...
			for(std::size_t r = 0; r < refvecs.size1(); ++r){
				double l = norm_2(row(refvecs, r));
				BOOST_CHECK_CLOSE(l, 1, 1e-11);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE(weightLattice_correct){
	for(std::size_t n = 1; n < 6; ++n){
		for(std::size_t sum = 3; sum < 10; ++sum){
			RealMatrix m = weightLattice(n, sum);
			BOOST_REQUIRE_EQUAL(m.size2(), n);
			BOOST_REQUIRE_EQUAL(m.size1(), detail::sumlength(n, sum));
			for(std::size_t row = 0; row < m.size1(); ++row){
				double actual_sum = 0;
				for(std::size_t col = 0; col < m.size2(); ++col){
					BOOST_CHECK_GE(m(row, col), 0.0);
					actual_sum += m(row, col);
				}
				BOOST_CHECK_CLOSE(actual_sum, 1.0, 1.e-12);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE(weightLattice_has_expected_size)
{
	for(std::size_t mu_prime = 3; mu_prime < 10; ++mu_prime)
	{
		for(std::size_t d = 2; d < 5; ++d)
		{
			RealMatrix l = weightLattice(d, mu_prime);
			std::size_t expected_size = detail::sumlength(d, mu_prime);
			BOOST_CHECK_EQUAL(expected_size, l.size1());
		}
	}
}

BOOST_AUTO_TEST_CASE(computeOptimalLatticeTicks_2d_correct)
{
	for(std::size_t i = 1; i < 100; ++i){
		const std::size_t pc = computeOptimalLatticeTicks(2, i);
		BOOST_CHECK_EQUAL(i, detail::sumlength(2, pc));
	}
}


BOOST_AUTO_TEST_CASE(sampleUniformly_correct){
	random::rng_type rng;
	for(std::size_t n = 2; n < 5; ++n){
		for(std::size_t sum = n; sum < 15; ++sum){
			RealMatrix m = weightLattice(n, sum);
			RealMatrix sampled = sampleLatticeUniformly(rng, m, sum);
			BOOST_CHECK_EQUAL(sampled.size1(), sum);
			std::size_t num_corners_orig = 0;
			std::size_t num_corners_sampled = 0;
			for(std::size_t r = 0; r < sampled.size1(); ++r){
				if(detail::isLatticeCorner(sampled.row_begin(r), sampled.row_end(r))){
					++num_corners_sampled;
				}
			}
			for(std::size_t r = 0; r < m.size1(); ++r)
			{
				if(detail::isLatticeCorner(m.row_begin(r), m.row_end(r)))
				{
					++num_corners_orig;
				}
			}
			BOOST_CHECK_EQUAL(num_corners_orig, n);
			BOOST_CHECK_EQUAL(num_corners_sampled, n);
		}
	}
}

BOOST_AUTO_TEST_CASE(best_point_count){
	for(std::size_t n = 3; n < 5; ++n)
	{
		for(std::size_t sum = 3; sum < 10; ++sum)
		{
			std::size_t b = computeOptimalLatticeTicks(n, sum);
			RealMatrix w = weightLattice(n, b);
			BOOST_CHECK(detail::sumlength(n, b) > sum);
			BOOST_CHECK_EQUAL(w.size1(), detail::sumlength(n, b));
		}
	}
}


BOOST_AUTO_TEST_CASE(vector_sorting_correct)
{
	const std::size_t mu_prime = 8;
	const std::size_t n_max = 4;
	for(std::size_t n = 2; n < n_max; ++n)
	{
		const RealMatrix weights = weightLattice(mu_prime, n);
		for(std::size_t T = 1; T <= weights.size1() / 2 && T <= 30; ++T)
		{
			RealMatrix dists = computeClosestNeighbourIndicesOnLattice(weights, T);
			for(std::size_t row = 0; row < dists.size1(); ++row)
			{
				std::list<std::vector<double>> my_nearest_points;
				std::for_each(dists.row_begin(row), dists.row_end(row),
							  [&](std::size_t idx)
							  {
								  my_nearest_points.push_back(
									  std::vector<double>(weights.row_begin(idx),
														  weights.row_end(idx)));
							  });
				BOOST_CHECK_EQUAL(my_nearest_points.size(), T);
				const std::vector<double> this_point(weights.row_begin(row),
													 weights.row_end(row));
				std::list<double> my_dists;
				for(std::vector<double> const & point : my_nearest_points)
				{
					double d = 0;
					for(std::size_t i = 0; i < point.size(); ++i)
					{
						d += std::pow(this_point[i] - point[i], 2);
					}
					my_dists.push_back(std::sqrt(d));
				}
				BOOST_CHECK_EQUAL(my_dists.size(), T);
				std::vector<double> my_dists_vec(my_dists.begin(), my_dists.end());
				for(std::size_t i = 1; i < my_dists_vec.size(); ++i)
				{
					double delta = my_dists_vec[i] - my_dists_vec[i - 1];
					// delta is negative if the vector is *not* sorted
					if(delta < 0)
					{
						// Check whether it's just due to floating point stuff.
						// (This is why we don't just use std::is_sorted)
						BOOST_CHECK_CLOSE_FRACTION(my_dists_vec[i],
												   my_dists_vec[i - 1], 1e-8);
					}
				}
			}
		}

	}
}


BOOST_AUTO_TEST_SUITE_END()
