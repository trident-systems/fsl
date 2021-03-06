#pragma once

#include <fsl/common.hpp>

#include <fsl/population/sex-age.hpp>
#include <fsl/harvesting/sex-age.hpp>
#include <fsl/monitoring/distribution-summary.hpp>

namespace Fsl {
namespace Monitoring {

template<class Time, class Age>
class AgeCatchSampling : public Structure< AgeCatchSampling<Time, Age> > {
public:

	Array<double, Time, Age> proportions;
	Array<DistributionSummary, Time> summaries;

	template<class Mirror>
    void reflect(Mirror& mirror) {
        mirror
            .data(proportions, "proportions")
            .data(summaries, "summaries")
        ;
    }

	void initialise() {

	}

	template<class Sex>
	void update(unsigned int time, const Population::SexAge<Sex, Age>& population, const Harvesting::SexAge<Sex, Age>& harvesting) {
		Array<double, Age> sample = 0;
		double sum = 0;
		for (auto age : Age::levels) {
			for (auto sex : Sex::levels) {
				sample(age) += population.numbers(sex, age) *
								harvesting.selectivities(sex, age);
			}
			sum += sample(age);
		}
		sample /= sum;
		for (auto age : Age::levels) proportions(time, age) = sample(age);
		summaries(time).calculate(sample);
	}

};

}
}
