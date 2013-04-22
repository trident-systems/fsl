/*
C++ code skeleton for FiSL management procedure evaluation.
*/

#include <fsl/management/procedures/evaluation.hpp>
using Fsl::Management::Procedures::Evaluation;

#include <fsl/population/models/model.hpp>
using Fsl::Population::Models::Aged;
using Fsl::Population::Models::Sexed;

#include <fsl/math/functions/function.hpp>
using Fsl::Math::Functions::Cached;

#include <fsl/population/growth/von-bert.hpp>
using Fsl::Population::Growth::VonBert;
#include <fsl/population/morphometry/morphometry.hpp>
using Fsl::Population::Morphometry::Power;
#include <fsl/population/maturity/maturity.hpp>
using Fsl::Population::Maturity::Logistic;
#include <fsl/population/recruitment/beverton-holt.hpp>
using Fsl::Population::Recruitment::BevertonHolt;
#include <fsl/population/mortality/rate.hpp>
using Fsl::Population::Mortality::Rate;

#include <fsl/math/probability/distributions.hpp>
using namespace Fsl::Math::Probability;

#include <fsl/population/recruitment/priors/variation-bentley-2012.hpp>
using Fsl::Population::Recruitment::Priors::VariationBentley2012;
#include <fsl/population/recruitment/priors/autocorrelation-bentley-2012.hpp>
using Fsl::Population::Recruitment::Priors::AutocorrelationBentley2012;

#include <fsl/monitoring/composition/composition.hpp>
using namespace Fsl::Monitoring;

#include <fsl/estimation/mcmc/metropolis.hpp>
using Fsl::Estimation::Mcmc::Metropolis;

using namespace Fsl;

class Fishery {
    FSL_PROPERTY(Fishery,catches,double)
    
    Logistic selectivities;
};

class BNS : public Evaluation<BNS> {
public:

    class Model {
    public:
        
        static const int ages = 80;
        typedef Sexed<
            Aged<
                ages, // Number of age classes
                double,// Type
                Cached<VonBert,ages>, // Sizes
                Cached<Power,ages>, // Weights
                Cached<Logistic,ages>, // Maturities
                int, // Recruitment
                Rate // Mortality
            >
        > Fish;
        Fish fish;
        
        class Fishing {
        public:
            Fishery line;
            Fishery trawl;
        } fishing;
        
    };
    Model model;
    
    BNS(void){
        conditioning.start = 1936;
        conditioning.finish = 2013;
    }
    
    //! @brief Time-independent log-likelihood calculation
    //!
    //! Usually just evaluates model parameter values with respect to prior probability distributions
    template<
        class Connection
    >
    void priors(Connection& connect){

        auto& fishing = model.fishing;
        auto& line = fishing.line;
        auto& trawl = fishing.trawl;
        
        auto& fish = model.fish;
        auto& males = fish.males;
        auto& females = fish.females;
        
        connect
            
            // Proportion of males
            __(fish.proportion,Fixed(0.5))
            
            // Steepness
            // Cordue & Pomarède (2012) : base case 0.75; sensitivity 0.9
            __(fish.recruitment.relationship.steepness,Fixed(0.75))

            // Variation
            // Cordue & Pomarède (2012) : 0.6
            __(fish.recruitment.variation.sd,VariationBentley2012())
            
            // Autocorrelation
            __(fish.recruitment.autocorrelation.coefficient, AutocorrelationBentley2012())
            
            // Instantaneous rate of natural mortality
            // Cordue & Pomerede (2012) : 0.08 for base case and did sensitivity analyses with 0.06 & 0.10
            //! @todo Consider using a 'flat-topped-triangular' distribution here
            __(males.mortality.instantaneous, Uniform(0.06,0.1))
            __(females.mortality.instantaneous, Uniform(0.06,0.1))
            
            // Size-at-age : von Bertalannfy function parameters
            // Horn et al (2010) estimates with 10% CV
            // Prior on cv of size-at-age same as used by Cordue & Pomarède (2012)
            //! @todo Implement distributions of size-at-age
            __(males.sizes.k, NormalCv(0.125,0.1))
            __(males.sizes.linf, NormalCv(72.2,0.1))
            __(males.sizes.t0, Fixed(-0.5))
            //males.sizes.cv(aide, Uniform(0.02,0.20));
            
            __(females.sizes.k, NormalCv(0.071,0.1))
            __(females.sizes.linf, NormalCv(92.5,0.1))
            __(females.sizes.t0, Fixed(-0.5))
            //females.sizes.cv(aide, Uniform(0.02,0.20));
            
            // Weight-at-size : power function parameters
            __(males.weights.a, NormalCv(0.00963,0.1))
            __(males.weights.b, NormalCv(3.173,0.1))
            
            __(females.weights.a, NormalCv(0.00963,0.1))
            __(females.weights.b, NormalCv(3.173,0.1))
            
            // Maturity-at-age : logistic function parameters
            __(males.maturities.inflection, NormalCv(15,0.1))
            __(males.maturities.steepness, NormalCv(5,0.1))
            
            __(females.maturities.inflection, NormalCv(17,0.1))
            __(females.maturities.steepness, NormalCv(10,0.1))
            
            // Selectivity-at-size 
            // Uniformative priors based on eyeballing length frequencies
            __(line.selectivities.inflection, Uniform(40,60))
            __(line.selectivities.steepness, Uniform(0,60))
            
            __(trawl.selectivities.inflection, Uniform(40,60))
            __(trawl.selectivities.steepness, Uniform(0,60))
        ;
    }
    
    template<
        class Aide
    >
    double likelihood(Aide& aide, int time){
        double likelihood = 0;
        
        auto& fishing = model.fishing;
        auto& line = fishing.line;
        auto& trawl = fishing.trawl;
        
        // Catches are defined by a high and low for each of trawl and line
        struct Catches {
            int year;
            struct range {
                float lo;
                float hi;
            };
            range line;
            range trawl;
        };
        static const Catches catches [] = {
            {1936,{0,150},{0,0}},
            {1937,{0,150},{0,0}},
            {1938,{0,150},{0,0}},
            {1939,{0,150},{0,0}},
            {1940,{0,112},{0,0}},
            {1941,{0,100},{0,0}},
            {1942,{0,100},{0,0}},
            {1943,{0,100},{0,0}},
            {1944,{0,100},{0,0}},
            {1945,{0,100},{0,0}},
            {1946,{0,138},{0,0}},
            {1947,{0,150},{0,0}},
            {1948,{0,162},{0,0}},
            {1949,{0,189},{0,0}},
            {1950,{0,177},{0,0}},
            {1951,{0,147},{0,0}},
            {1952,{0,142},{0,0}},
            {1953,{0,141},{0,0}},
            {1954,{0,137},{0,0}},
            {1955,{0,132},{0,0}},
            {1956,{0,138},{0,0}},
            {1957,{0,138},{0,0}},
            {1958,{0,149},{0,0}},
            {1959,{0,137},{0,0}},
            {1960,{0,124},{0,0}},
            {1961,{0,121},{0,0}},
            {1962,{0,118},{0,0}},
            {1963,{0,119},{0,0}},
            {1964,{0,133},{0,0}},
            {1965,{0,128},{0,0}},
            {1966,{0,123},{0,0}},
            {1967,{0,129},{0,0}},
            {1968,{0,113},{0,0}},
            {1969,{0,111},{0,0}},
            {1970,{0,140},{0,0}},
            {1971,{0,138},{0,0}},
            {1972,{0,118},{0,78}},
            {1973,{0,126},{0,72}},
            {1974,{0,137},{0,117}},
            {1975,{111,252},{0,204}},
            {1976,{618,767},{0,211}},
            {1977,{821,1004},{0,1505}},
            {1978,{1,161},{0,0}},
            {1979,{9,176},{0,0}},
            {1980,{15,180},{0,0}},
            {1981,{235,365},{0,0}},
            {1982,{469,554},{0,0}},
            {1983,{730,780},{0,0}},
            {1984,{951,962},{324,324}},
            {1985,{1013,1013},{372,372}},
            {1986,{982,982},{605,605}},
            {1987,{744,744},{667,667}},
            {1988,{752,752},{522,522}},
            {1989,{797,797},{623,623}},
            {1990,{777,777},{763,763}},
            {1991,{1192,1192},{577,577}},
            {1992,{1414,1414},{549,549}},
            {1993,{1573,1573},{733,733}},
            {1994,{1459,1459},{860,860}},
            {1995,{1382,1382},{904,904}},
            {1996,{1503,1503},{811,811}},
            {1997,{1765,1765},{1060,1060}},
            {1998,{1728,1728},{779,779}},
            {1999,{1871,1871},{904,904}},
            {2000,{1712,1712},{1022,1022}},
            {2001,{1638,1638},{1082,1082}},
            {2002,{1443,1443},{1345,1345}},
            {2003,{1671,1671},{1331,1331}},
            {2004,{2133,2133},{957,957}},
            {2005,{1900,1900},{1114,1114}},
            {2006,{1765,1765},{710,710}},
            {2007,{2001,2001},{424,424}},
            {2008,{2000,2000},{500,500}},
            {2009,{1746,1746},{300,300}},
            {2010,{1759,1759},{300,300}},
            {2011,{1700,1700},{300,300}},
            {2012,{1700,1700},{300,300}},
            {2013,{1700,1700},{300,300}},
        };
        auto catche = catches[time-1936];
        line.catches(aide,Uniform(catche.line.lo,catche.line.hi));
        trawl.catches(aide,Uniform(catche.trawl.lo,catche.trawl.hi));
        
        // CPUE
        struct CPUEs {
            int year;
            float line;
            float trawl;
        };
        const CPUEs cpues[] = {
            {1990,1.247441814,1.510256596},
            {1991,1.590947955,1.042574728},
            {1992,1.477727473,1.894356836},
            {1993,1.649550216,1.750991576},
            {1994,1.500194891,1.899914374},
            {1995,1.193148238,0.963911837},
            {1996,1.250706938,1.053177411},
            {1997,1.457456864,1.2284709},
            {1998,1.314037285,1.232794605},
            {1999,1.168289555,1.290068485},
            {2000,1.152326268,0.933782045},
            {2001,1.113044069,1.190904953},
            {2002,1.098104608,1.249063247},
            {2003,1.216502307,0.977561748},
            {2004,1.100227489,0.878536218},
            {2005,0.753376718,0.951121731},
            {2006,0.614730612,0.557849976},
            {2007,0.545121426,0.484998026},
            {2008,0.434553641,0.551232474},
            {2009,0.464832065,0.545266918},
            {2010,0.429294754,0.550689277},
        };
        auto cpue = cpues[time-1936];
        
        /*static const Composition::Sample<19,121,2> line_lengths[16] = {
            {1993,10,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.000833034,0.01907067,0.06234897,0.05787144,0.09205867,0.07474554,0.07462903,0.06442023,0.05551416,0.06665473,0.06177136,0.05009846,0.05755423,0.03631356,0.04637422,0.04059342,0.02823511,0.02532275,0.01646804,0.01509342,0.01095231,0.01054003,0.005978919,0.004740671,0.008281408,0.002961299,0.003867287,0.003174747,0.001295687,0.001050167,0.0007580613,0,0.0001366856,0,0.0001470798,0}},
            {1994, 2,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.002088497,0,0,0.01118551,0.0386302,0.04073961,0.03668246,0.0587458,0.06882336,0.0844908,0.1339844,0.08683745,0.07804843,0.094172,0.04425235,0.04293688,0.03625063,0.03115765,0.02785656,0.01417502,0.02453844,0.009071525,0.01535035,0.004280811,0.006553846,0.001712733,0.004409622,0.001821751,0.0003476381,0.0008556447,0,0,0,0,0,0,0}},
            {1995, 4,{0,0,0,0,0,0,0,0,0,0,0,0,0.001951494,0.006970228,0.05123687,0.0747149,0.04020342,0.0635675,0.06250192,0.0825977,0.071601,0.09895632,0.06510201,0.04441395,0.06159359,0.04387734,0.0431837,0.01933916,0.01585675,0.01469333,0.01314635,0.01921609,0.01862644,0.01324993,0.01091688,0.01166447,0.009225448,0.006070978,0.006858778,0.007866656,0.006880516,0.004725787,0.002802254,0.0008428806,0.002679879,0.0004551937,0.001804038,0,0.0006062384,0,0}},
            {1996, 7,{0,0,0,0,0,0,0,0,0,0,0,0.002439013,0,0.0005101933,0.02847421,0.07256657,0.05578062,0.05787344,0.05574118,0.08573875,0.1192315,0.1078919,0.09660355,0.08516509,0.0667029,0.0502352,0.03425016,0.02550657,0.01602009,0.01089069,0.007679021,0.005706875,0.003067761,0.001881337,0.003181110,0.002389083,0.0008685203,0.0003567525,0.0005825793,0.0008078353,1.271716e-05,0.0002179089,0,0.001043853,0.0003524023,0,0.0002179089,0,0,0,1.271716e-05}},
            {1997,32,{0,0,0,0,0,0,3.912111e-05,0.0001877923,0,0.001841102,0.002232748,0.004817867,0.005361016,0.009550732,0.02959137,0.06483232,0.0671129,0.09111494,0.0871997,0.0864397,0.08344914,0.08045222,0.08244177,0.05983338,0.05656332,0.04126666,0.03461616,0.02007929,0.01723191,0.01629499,0.01610065,0.008948374,0.00783506,0.007987189,0.005295748,0.003342567,0.002106076,0.001466373,0.001252883,0.0009473101,0.000973236,0.0002775681,0.0002653043,0.0002289464,0.0002619671,4.44539e-05,0,0.0001161277,0,0,0}},
            {1998,19,{0,0,0,0,0,0,0,0,0,0.0002080367,0,0.0001360276,0.0004181825,0.005271027,0.02719975,0.05340558,0.07430036,0.0864845,0.087711,0.09841176,0.09516996,0.08887776,0.08438755,0.06132509,0.06345817,0.03899578,0.02464859,0.02353183,0.01172220,0.01549702,0.00940414,0.00957537,0.009239372,0.007488616,0.004109825,0.004288218,0.003448511,0.004120912,0.003227495,0.0003192350,0.0008565428,0.0007052956,0.0003669328,0.0007811777,0.0002715160,0.0002517235,0.0001212468,0.0002637077,0,0,0}},
            {1999,20,{0,2.035576e-05,0,0,0,0,0,0,4.071151e-05,0.000187939,0.001472152,0.001735844,0.003038184,0.003071571,0.02890338,0.07013204,0.0805171,0.08845659,0.09940573,0.1084639,0.09899896,0.081137,0.08617487,0.06066383,0.04134431,0.03652042,0.026575,0.01542838,0.01220077,0.009315495,0.01170378,0.006728284,0.00964164,0.004094407,0.005272029,0.003453921,0.0009933072,0.001180376,0.0008083183,0.0002325269,0.0002784671,0.0002243698,0,0.0003653545,0.0003667486,0.000791085,6.081429e-05,0,0,0,0}},
            {2000,14,{0,0,0,0,0,0,0.001645453,0.0006195374,0.002462247,0.003064009,0.002035381,0.003528527,0.001246496,0.009441382,0.04973815,0.1016881,0.1004298,0.0826693,0.1059320,0.08914305,0.09482094,0.08131894,0.07784081,0.0399449,0.03808148,0.0325412,0.01853448,0.01185210,0.008537929,0.01048621,0.007076915,0.004310271,0.004258643,0.002828536,0.002965858,0.003710454,0.001597613,0.001032728,0.001025803,0.0009886237,0.0002874664,0.000177379,0.0001392196,0.000729877,0.000644862,0.0003366853,0.0002651314,0,0,0,0}},
            {2001,14,{0,0,0,0,0,0.001200278,0.003460784,0.001172576,0.003429272,0.003692143,0.004412721,0.003136546,0.001343542,0.01792959,0.07926287,0.1540561,0.1679265,0.1079745,0.0883399,0.06255923,0.07859732,0.06660509,0.04183325,0.02669441,0.01524119,0.01941711,0.01379246,0.007254297,0.009138866,0.007622425,0.002658743,0.001523725,0.002608082,0.002400284,0.002174176,0.001004692,0.0004872101,0.000323458,9.013154e-05,0.0005214016,1.267249e-05,0,0,0,0,0,0,0.0001024305,0,0,0}},
            {2002, 8,{0,0.01254557,0,0.01254557,0.001014284,0.03304973,0.01873846,0.003269959,0.01726231,0.04661792,0.03907993,0.03685197,0.02194842,0.04677307,0.06071196,0.09819849,0.09160194,0.0662933,0.0575858,0.03659285,0.05949228,0.06965559,0.04843567,0.03716772,0.02909325,0.01493039,0.00624949,0.00595732,0.01041636,0.003593370,0.002699171,0.002292237,0.002108238,0.003219974,0.00100714,0.0007224421,0.000948807,0.0002818598,0.0003093084,0.0003519134,0,0,0,0.0003859575,0,0,0,0,0,0,0}},
            {2003, 4,{0,0.0009967001,0,0,0,0.0253126,0.01487488,0.006519233,0.00835565,0.03188688,0.0540221,0.01043772,0.03420224,0.002075497,0.04624752,0.06865826,0.05656626,0.06335071,0.06936536,0.05727059,0.06840694,0.04677754,0.05728594,0.03691287,0.01986714,0.03832593,0.03548033,0.03474119,0.02570534,0.02097603,0.01984831,0.007846012,0.002980477,0.001823910,0.002167627,0.005398813,0.001279773,0.004979329,0.005712703,0.003750032,0,0,0,0,0,0,0,0,0,0.003735815,0}},
            {2004, 3,{0.006495217,0,0,0,0,0,0.009025305,0,0.009025305,0.01127712,0.03551906,0.01417183,0.01013894,0.01026870,0.01698568,0.04299337,0.05157036,0.06858243,0.06956839,0.07500855,0.1080621,0.07490781,0.08156955,0.04238332,0.04606704,0.04294545,0.04990923,0.01853842,0.0213014,0.01883495,0.01063781,0.01820429,0.00721752,0.009443702,0.003029309,0.002291640,0.002348318,0.003394647,0.0006677748,0.002448124,0.002385301,0.002059969,0,0,0,0.0007220154,0,0,0,0,0}},
            {2005, 4,{0,0,0,0,0,0,0,0,0,0,0,0.001374136,0.0003919997,0.0009593593,0.01625565,0.0392564,0.0534077,0.08169088,0.07756105,0.1089275,0.1032241,0.1187824,0.1094815,0.08083867,0.0570876,0.0434258,0.02765646,0.02138807,0.007293316,0.01721544,0.009283102,0.007614942,0.001532345,0.001341824,0.00447502,0.001994104,0.002627414,0.001116653,0.000555307,0.0009772627,0.0003271633,0.0001457229,0.0002812737,0.0007337297,0.0002479523,0,0,0,0,0,0.0001056350}},
            {2006, 6,{0,0,0,0,0,0,0,0,1.118206e-05,0,0.0001358196,0,0,0,0.0004356601,0.02089889,0.0924201,0.07464896,0.1665842,0.1312537,0.09719674,0.09858058,0.108905,0.045692,0.05177953,0.02354293,0.02193468,0.01568127,0.006974484,0.02108778,0.003428229,0.003325304,0.001890497,0.0007110101,0.002315411,0.004167773,0.001501069,0.001808627,0.001333887,0.0005952916,0.0005992896,0.0002018768,9.962319e-05,0.0002585633,0,0,0,0,0,0,0}},
            {2007, 8,{0,0,0,0,0,0,0,0,0,0,0,0,0,0.01372076,0.03743546,0.1030029,0.08709071,0.07538374,0.0620374,0.06411771,0.05755508,0.06313634,0.07717764,0.05973588,0.06378247,0.05062632,0.03956377,0.02898915,0.02039481,0.01102126,0.01270849,0.008389985,0.01157540,0.007763662,0.01608820,0.008995939,0.005763921,0.004451052,0.003437062,0.002257977,0.001204630,0.000918174,0.0005841044,0.0007674782,0,0,0,1.330027e-05,0.0002959331,1.330027e-05,0}},
            {2008, 8,{0,0,0,0,0,0,0,0,0,0,0,0,0,0.003411721,0.0255222,0.09457644,0.1207092,0.1207135,0.0946572,0.09570498,0.08473751,0.07118058,0.07757383,0.03784860,0.02427679,0.02235321,0.01903186,0.02376668,0.02203408,0.009871937,0.01098032,0.004785387,0.004065826,0.003256392,0.002226485,0.003036518,0.001084377,0.004871237,0.004265542,0.007898112,0.003363144,0.001601792,0,0,0,0.0005945635,0,0,0,0,0}}
        };*/
        
        //static const Composition::Sample<19,121,2> trawl_lengths[1] = {
            //{1995, 17,{0,0,0,0,0,0,0,0,0,0,0,0.005386854,0,0,0.004395466,0.04199339,0.0645052,0.02791055,0.05898416,0.1018713,0.1369029,0.067774,0.1153757,0.122697,0.07824038,0.03888739,0.0410989,0.01585228,0.02495305,0.01320923,0.008759512,0.008558257,0.002305707,0.005695989,0.004088632,0.00280899,0.003484397,0,0,0.000993046,0.002342325,0,0,0.00092533,0,0,0,0,0,0,0}},
            //{1996, 21,{0,0,0,0,0,0,0,0,0,0,0,0,0.001533445,0,0.009428641,0.01818692,0.05292114,0.05475225,0.06317558,0.0482625,0.06484054,0.08293471,0.134773,0.0807305,0.07530848,0.06927263,0.05837092,0.04329575,0.01508776,0.02009997,0.02493512,0.01156858,0.02076003,0.007934844,0.01116985,0.007974544,0.003535176,0.01380494,0,0,0.000807824,0.003354132,0,0.001180252,0,0,0,0,0,0,0}},
            //{1997, 13,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.006785822,0.015046,0.04525421,0.074516,0.07069951,0.0894621,0.02937663,0.1041428,0.1119376,0.06046075,0.1497293,0.03590485,0.06513476,0.05977406,0.005269833,0.03237689,0.003015264,0.02963404,0.003817985,0.001459142,0.000897775,0.002379543,0,0.001601162,0,0,0.00082926,0,0,0,0.000494716,0,0,0,0,0,0}},
            //{1998, 81,{0,0,0,0,0,0,0,0,0,0,0,0.001934453,0.002173173,0.002297901,0.008470545,0.0403711,0.05762125,0.05974484,0.05747124,0.08453248,0.0993494,0.080777,0.08258507,0.06894717,0.1000575,0.05510098,0.06750686,0.03088648,0.02106523,0.01326274,0.01120687,0.01076367,0.01027152,0.00617681,0.007101847,0.001623326,0.00149393,0.004442368,0.004571129,0.000211418,0.000409106,0.004587499,0.000116488,0.002528844,0.000116488,0.000115583,0,0,0,0.000107706,0}},
            //{1999, 35,{0,0,0,0,0,0,0,0,0,0,0,0.001312261,0.001155765,0,0.03756868,0.1553176,0.1580496,0.09945655,0.09338815,0.0698425,0.07824198,0.08273436,0.04720039,0.03152124,0.01812585,0.01671664,0.01402283,0.01144996,0.01830753,0.01257218,0.009511364,0.01269004,0.008683994,0.004844609,0.002383933,0.004741205,0.003556566,0.002064696,0.001775518,0.002038642,0.000725359,0,0,0,0,0,0,0,0,0,0}},
            //{2000, 59,{0,0,0,0,0,0,0,0,0,0,0.005869002,0,0,0.007456609,0.01469892,0.02823726,0.02836204,0.0757227,0.09347506,0.08332769,0.08656081,0.09830502,0.1088878,0.0587462,0.0963137,0.0335986,0.02986388,0.01990359,0.04784148,0.01618276,0.01736213,0.009012572,0.01166606,0.008983441,0.00517429,0.003392531,0.004073925,0.002896561,0.000420024,0.001595957,0.000850399,0,0,0.001029484,0.00018945,0,0,0,0,0,0}},
            //{2001, 77,{0,0,0,0,0,0,0,0,0,0,0,0.001088462,0.006698544,0.01261797,0.01687603,0.04561997,0.036877,0.05313648,0.05282115,0.05734986,0.06497931,0.07707983,0.08853574,0.076246,0.07449724,0.07360313,0.07587181,0.05765842,0.04363877,0.02399875,0.02921263,0.01007713,0.007678843,0.003527724,0.002844199,0.001390611,0.001993638,0.001306159,0.000839437,0.000807397,0.000683816,0,0.000443961,0,0,0,0,0,0,0,0}},
            //{2002,100,{0,0.001437519,0,0,0,0.00904225,0,0.002142891,0.001437519,0.000390726,0,0,0.000520523,0.000520523,0.005705558,0.02694496,0.06408431,0.05801856,0.08395478,0.07914624,0.08917436,0.08658898,0.08793863,0.06364827,0.07059853,0.0721953,0.05757843,0.05365516,0.01847028,0.01747015,0.01873577,0.02163587,0.002120954,0.000925039,0.002499875,0.00080209,0.000849523,0.000330847,0.000801764,0.000110987,0.000189984,0,0.000116077,0.000105768,0.000110987,0,0,0,0,0,0}},
            //{2003, 57,{0,0,0,0,0,0,0.05971417,0.02909296,0.05971417,0.01133514,0.03459166,0.0359807,0,0.007843138,0.01603625,0.05787173,0.0786105,0.1035397,0.08014267,0.1462817,0.1058573,0.06336534,0.04955728,0.01423265,0.0131219,0.007054365,0.003896355,0.004075708,0.003353946,0.003359487,0.003522022,0.002212726,0.001747447,0.001737301,0.000576918,0.000686064,0.000245303,0.000199293,0.000184217,2.98E-05,0.000193751,3.63E-05,0,0,0,0,0,0,0,0,0}},
            //{2004,104,{0,0,0,0,0,0,0,0,0,0,0.000304247,0.000304247,0.001407648,0.000904407,0.007322961,0.03180188,0.04374591,0.04990059,0.09708846,0.1507301,0.108393,0.1200567,0.08300504,0.06018672,0.03575333,0.03189492,0.02027619,0.025169,0.02076878,0.0259457,0.02368006,0.01421261,0.02100907,0.01039054,0.003579549,0.001829656,0.002068277,0.004283432,0.00214188,0,0.00174496,0.000100185,0,0,0,0,0,0,0,0,0}},
        //};
        
        //Palliser Bank 1986 catch-at-age
        //static const Composition::Sample<5,50> trawl_ages[1] = {
        //    {1986,200,{0.0003,0.003,0.0125,0.0129,0.018,0.0311,0.0336,0.0449,0.0477,0.0334,0.0394,0.0346,0.0288,0.0548,0.0412,0.0348,0.0443,0.0381,0.0616,0.0541,0.0442,0.0433,0.0426,0.0404,0.0302,0.0265,0.0187,0.0233,0.0103,0.01,0.0055,0.0098,0.0059,0.0055,0.0021,0.0023,0.0021,0.0014,0.002,0.0007,0.0013,0.0007,0.0005,0.0002,0.0003,0.0011}}
        //};
        
        //Bay of Plenty/East Northland 2001 catch-at-age
        //static const Composition::Sample<4,38> line_ages[1] = {
        //    {2001,200,{6.00E-04,0.0178,0.0168,0.0622,0.0933,0.0938,0.0822,0.0688,0.0841,0.0384,0.0449,0.0475,0.0438,0.0406,0.042,0.048,0.0281,0.0214,0.0209,0.0223,0.0105,0.0086,0.0119,0.0107,0.0126,0.0032,0.0061,0.0022,0.0049,0.0022,0.0025,0.0046,0.0011,0,0.0012}},
        //};
        
        return likelihood;
    }

} mpe;

int main(void){
    mpe.run();
}
