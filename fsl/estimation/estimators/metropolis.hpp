#pragma once

#include <fstream>

#include <stencila/arrays.hpp>
#include <stencila/tables.hpp>

#include <fsl/math/probability/uniform.hpp>
#include <fsl/math/probability/normal.hpp>

namespace Fsl {
namespace Estimation {
namespace Mcmc {
    
using namespace Stencila::Arrays;
using namespace Stencila::Tables;
using Stencila::Real;
using Stencila::Integer;

/*!
@brief Metropolis Markov chain Monte Carlo algorithm

The Metropolis algorithm is probably the simplest Markov chain Monte Carlo (MCMC)
algorithm. It is a special, simplified case of the Hastings-Metropolis algorithm in
which the proposal jump distribution is symetric and therefore the acceptance
ratio for a jump is simply the ratio of the likelihoods.

This implementation uses the gaussian distribution

@author Nokome Bentley
*/
template<
    class Derived,
    int Parameters
>
class Metropolis {
public:
    //! Vector that represents the paramter values at the end of the chain
    Array<> values = Parameters;
    double ll;
    
    //! Variances for proposals. These need to be tuned
    //! by user to obtain an acceptance ratio (20-70%)
    Array<> variances = Parameters;
    
    unsigned int iterations;
    unsigned int accepted;
    double acceptance;
    
    Table samples;
    
    Metropolis(void):
        samples("samples",
            "iteration",Real,
            "acceptance",Real,
            "log_like",Real
        )
    {
        //! @todo add all parameters
        for(int par=0;par<Parameters;par++) samples.add("p"+boost::lexical_cast<std::string>(par),Real);
    }
    
    void reset(void){
        iterations = 0;
        accepted = 0;
        acceptance = 0;
        ll = -INFINITY;
    }
    
    void step(void){
        typedef Math::Probability::Normal Normal;
        typedef Math::Probability::Uniform Uniform;
        
        //! Generate a proposal for each parameter
        Array<> proposal(Parameters);
        for(int par=0;par<Parameters;par++){
            double value = values(par);
            double variance = variances(par);
            proposal(par) = Normal(value,std::sqrt(variance)).random();
        }
        //! Obtain likelihood for proposal
        double ll_proposal = static_cast<Derived*>(this)->log_like(proposal);
        //! Test to see if this proposal will be accepted
        double ratio = ll_proposal-ll;
        iterations++;
        if(ratio>std::log(Uniform().random())){
            values = proposal;
            ll = ll_proposal;
            
            iterations++;
            acceptance = accepted/float(iterations);
            
            //Insert the current values into the samples datatable
            std::vector<double> row(3+Parameters);
            row[0] = iterations;
            row[1] = acceptance;
            row[2] = ll;
            for(int par=0;par<Parameters;par++) row[3+par] = values(par);
            samples.append(row);
        }
    }
    
    void run(unsigned int n){
        for(unsigned int i=0;i<n;i++){
            step();
        }
    }
};

} // end namespace Mcmc
} // end namespace Estimation
} // end namespace Fsl
