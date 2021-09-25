#include <RcppArmadillo.h>
#include <RcppArmadilloExtensions/sample.h>
#include <vector>

// [[Rcpp::plugins("cpp11")]]
// [[Rcpp::depends("RcppArmadillo")]]

arma::uword sample (const arma::vec& pvec) 
{
  arma::uword K = pvec.n_elem;
  arma::uvec opts = arma::linspace<arma::uvec>(0L, K - 1L, K);
  return arma::conv_to<arma::uword>::from(
      Rcpp::RcppArmadillo::sample(opts, 1L, true, pvec)
  );
}

// [[Rcpp::export]]
double genotyping_error_model 
 (const arma::uvec& phenotype,
  const unsigned& genotype0, 
  const unsigned& genotype1, 
  const unsigned& number_of_alleles,
  const double& dropout_rate, 
  const double& mistyping_rate)
{
  // from Eqs 1 & 2 in Wang 2004 Genetics
  const double e1 = dropout_rate;
  const double e2 = mistyping_rate/double(number_of_alleles-1);
  const double E2 = mistyping_rate;

  const arma::uvec genotype = {genotype0, genotype1};
  const bool phenotype_is_homozygous = phenotype[0] == phenotype[1];
  const bool genotype_is_homozygous = genotype[0] == genotype[1];

  if (number_of_alleles == 1) return 1.; //monomorphic loci

  if (genotype_is_homozygous) 
  {
    if (phenotype_is_homozygous && phenotype[0] == genotype[0])
    {
      return std::pow(1.-E2, 2);
    } else if ((phenotype[0] == genotype[0] && phenotype[1] != genotype[0]) || 
               (phenotype[0] != genotype[0] && phenotype[1] == genotype[0]) ){
      return 2.*e2*(1-E2);
    } else if (phenotype[0] != genotype[0] && phenotype[1] != genotype[0]) {
      return (2.-int(phenotype_is_homozygous))*std::pow(e2, 2);
    }  
  } else {
    if ( (phenotype[0] == genotype[0] && phenotype[1] == genotype[1]) ||
         (phenotype[1] == genotype[0] && phenotype[0] == genotype[1]) )
    {
      return std::pow(1.-E2, 2) + std::pow(e2, 2) - 2.*e1*std::pow(1.-E2-e2, 2);
    } else if (phenotype_is_homozygous && (phenotype[0] == genotype[0] || phenotype[0] == genotype[1])) {
      return e2*(1.-E2) + e1*std::pow(1.-E2-e2, 2);
    } else if (phenotype[0] != genotype[0] && phenotype[0] != genotype[1] && 
               phenotype[1] != genotype[0] && phenotype[1] != genotype[1] ){
      return (2.-int(phenotype_is_homozygous))*std::pow(e2, 2);
    } else {
      // Wang 2018 has (1-E2-e2) and Wang 2004 has (1-E2+e2), latter is correct
      return e2*(1.-E2+e2);
    }
  }
  return 0.;
}

// [[Rcpp::export]]
int genotyping_error_model_class
 (const arma::uvec& phenotype,
  const unsigned& genotype0, 
  const unsigned& genotype1) 
{
  // from Eqs 1 & 2 in Wang 2004 Genetics
  const arma::uvec genotype = {genotype0, genotype1};
  const bool phenotype_is_homozygous = phenotype[0] == phenotype[1];
  const bool genotype_is_homozygous = genotype[0] == genotype[1];

  if (genotype_is_homozygous) 
  {
    if (phenotype_is_homozygous && phenotype[0] == genotype[0])
    {
      return 1;
    } else if ((phenotype[0] == genotype[0] && phenotype[1] != genotype[0]) || 
               (phenotype[0] != genotype[0] && phenotype[1] == genotype[0]) ){
      return 2;
    } else if (phenotype[0] != genotype[0] && phenotype[1] != genotype[0]) {
      return 3;
    }  
  } else {
    if ( (phenotype[0] == genotype[0] && phenotype[1] == genotype[1]) ||
         (phenotype[1] == genotype[0] && phenotype[0] == genotype[1]) )
    {
      return 4;
    } else if (phenotype_is_homozygous && (phenotype[0] == genotype[0] || phenotype[0] == genotype[1])) {
      return 5;
    } else if (phenotype[0] != genotype[0] && phenotype[0] != genotype[1] && 
               phenotype[1] != genotype[0] && phenotype[1] != genotype[1] ){
      return 6;
    } else {
      return 7;
    }
  }
  return 0;
}

// [[Rcpp::export]]
arma::uvec simulate_genotyping_errors
 (const arma::uvec& phenotype,
  const unsigned& genotype0, 
  const unsigned& genotype1, 
  const unsigned& number_of_alleles,
  const double& dropout_rate, 
  const double& mistyping_rate)
{
  const double e1 = dropout_rate;
  const double e2 = mistyping_rate/double(number_of_alleles-1);
  const double E2 = mistyping_rate;

  const arma::uvec genotype = {genotype0, genotype1};
  const bool phenotype_is_homozygous = phenotype[0] == phenotype[1];
  const bool genotype_is_homozygous = genotype[0] == genotype[1];

  if (number_of_alleles == 1) return arma::uvec({0,0}); //monomorphic loci

  if (genotype_is_homozygous) 
  {
    if (phenotype_is_homozygous && phenotype[0] == genotype[0])
    {
      return arma::uvec({0, 0});
    } else if ((phenotype[0] == genotype[0] && phenotype[1] != genotype[0]) || 
               (phenotype[0] != genotype[0] && phenotype[1] == genotype[0]) ){
      return arma::uvec({0, 1});
    } else if (phenotype[0] != genotype[0] && phenotype[1] != genotype[0]) {
      return arma::uvec({0, 2});
    }  
  } else {
    if ( (phenotype[0] == genotype[0] && phenotype[1] == genotype[1]) ||
         (phenotype[1] == genotype[0] && phenotype[0] == genotype[1]) )
    {
      // either: no errors occurred OR 
      //         there was no dropout error but two typing errors
      //         there was a dropout error that got fixed by single typing error
      // prop.table(c((1-2*e1)*(1-E2)^2, (1-2*e1)*e2*e2, 4*e1*e2*(1-E2)))
      // sum(c((1-2*e1)*(1-E2)^2, (1-2*e1)*e2*e2, 4*e1*e2*(1-E2)))
      // (1-E2)^2 + e2^2 - 2*e1*(1-E2-e2)^2
      // [0,0] [0,2] [1,1]
      const arma::vec probs = {(1-2*e1)*(1-E2)*(1-E2), (1-2*e1)*e2*e2, 4*e1*e2*(1-E2)};
      const arma::umat counts = {{0,0},{0,2},{1,1}};
      return counts.row(sample(probs)).t();
    } else if (phenotype_is_homozygous && (phenotype[0] == genotype[0] || phenotype[0] == genotype[1])) {
      // one class 2 error OR there was a dropout error and no typing errors
      // prop.table(c(e1*(1-E2)^2, (1-2*e1)*e2*(1-E2), e1*e2*e2))
      // sum(c(e1*(1-E2)^2, (1-2*e1)*e2*(1-E2), e1*e2*e2))
      // e2*(1-E2) + e1*(1-E2-e2)^2
      // [0 1] [1 0] [1 2]
      const arma::vec probs = {e1*(1-E2)*(1-E2), (1-2*e1)*e2*(1-E2), e1*e2*e2};
      const arma::umat counts = {{1,0},{0,1},{1,2}};
      return counts.row(sample(probs)).t();
    } else if (phenotype[0] != genotype[0] && phenotype[0] != genotype[1] && 
               phenotype[1] != genotype[0] && phenotype[1] != genotype[1] ){
      // two class 2 errors occurred, regardless of whether class 1 error occurs
      // prop.table(c( (1-2*e1)*e2*e2 , 2*e1*e2*e2 ))
      // [0 2] [1 2]
      const arma::vec probs = {(1-2*e1)*e2*e2, 2*e1*e2*e2};
      const arma::umat counts = {{0,2},{1,2}};
      return counts.row(sample(probs)).t();
    } else {
      // "otherwise" ... there's one match but phenotype is heterozygous?
      // 1. could have: sequencing error at one, no sequencing error at other
      // 2. sequencing error gets fixed by second sequencing error
      // dropout could happen in either case
      // prop.table(c( (1-2*e1)*e2*(1-E2), (1-2*e1)*e2*e2, 2*e1*e2*(1-E2), 2*e1*e2*e2 ))
      // [0 1] [0 2] [1 1] [1 2]
      const arma::vec probs = {(1-2*e1)*e2*(1-E2), (1-2*e1)*e2*e2, 2*e1*e2*(1-E2), 2*e1*e2*e2};
      const arma::umat counts = {{0,1},{0,2},{1,1},{1,2}};
      return counts.row(sample(probs)).t();
    }
  }
  return arma::uvec{{0,0}};
}

arma::vec sample_error_rates_given_paternity_by_locus
 (const arma::uvec& paternity,
  const arma::umat& offspring_phenotypes, 
  const arma::uvec& maternal_phenotype, 
  const arma::vec& allele_frequencies, 
  const double& dropout_rate, 
  const double& mistyping_rate)
{
  // simulate from conditional posterior of offspring and maternal genotypes given phenotypes
  const unsigned number_of_alleles = allele_frequencies.n_elem;
  const arma::uvec fathers = arma::unique(paternity);
  const arma::vec allele_frequencies_normalized = allele_frequencies / arma::accu(allele_frequencies);

  if (offspring_phenotypes.n_rows != 2) Rcpp::stop("offspring phenotypes must have 2 rows");
  if (offspring_phenotypes.n_cols != paternity.n_elem) Rcpp::stop("offspring phenotypes must have column for each individual");
  if (maternal_phenotype.n_elem != 2) Rcpp::stop("maternal phenotype must have 2 elements");
  if (offspring_phenotypes.max() >= number_of_alleles) Rcpp::stop("offspring allele out of range");
  if (maternal_phenotype.max() >= number_of_alleles) Rcpp::stop("maternal allele out of range");
  if (arma::any(allele_frequencies_normalized < 0.)) Rcpp::stop("negative allele frequencies");
  if (dropout_rate <= 0. || mistyping_rate <= 0.) Rcpp::stop("negative genotyping error rates");

  // alternatively pass in as mutable argument
  arma::uvec maternal_genotype (2);
  arma::umat offspring_genotypes (2, paternity.n_elem);
  arma::uvec paternal_genotypes (fathers.n_elem);
  maternal_genotype.fill(arma::datum::nan);
  offspring_genotypes.fill(arma::datum::nan);
  paternal_genotypes.fill(arma::datum::nan);

  // simulate maternal genotype; 
  // we marginalize over paternal&offspring genotypes to calculate marginal maternal genotype probabilities
  unsigned number_of_genotypes = number_of_alleles*(number_of_alleles + 1)/2;
  arma::vec maternal_genotype_posterior (number_of_genotypes);
  arma::umat possible_maternal_genotypes (2, number_of_genotypes);
  unsigned genotype = 0;
  for (unsigned w=0; w<number_of_alleles; ++w) // first maternal allele 
  { 
    for (unsigned v=w; v<number_of_alleles; ++v) // second maternal allele
    {
      double maternal_genotype_probability = //1; //uniform prior
        (2.-int(w==v)) * allele_frequencies_normalized[w] * allele_frequencies_normalized[v]; //hwe prior
      double log_halfsib_likelihood = log(maternal_genotype_probability); 
      if (maternal_phenotype.is_finite())
      {
        double maternal_phenotype_probability = 
          genotyping_error_model(maternal_phenotype, w, v, number_of_alleles, dropout_rate, mistyping_rate);
        log_halfsib_likelihood += log(maternal_phenotype_probability);
      }
      for (auto father : fathers)
      {
        double fullsib_likelihood = 0.;
        arma::uvec offspring_from_father = arma::find(paternity == father);
        for (unsigned u=0; u<number_of_alleles; ++u) // paternal allele
        {
          double paternal_genotype_probability = //1; //uniform prior
            allele_frequencies_normalized[u]; //hwe prior
          double log_fullsib_likelihood = log(paternal_genotype_probability);
          for (auto offspring : offspring_from_father)
          {
            arma::uvec offspring_phenotype = offspring_phenotypes.col(offspring);
            if (offspring_phenotype.is_finite()) { 
              double offspring_phenotype_probability = // Mendelian segregation probs * phenotype probabilities
                0.5 * genotyping_error_model(offspring_phenotype, w, u, number_of_alleles, dropout_rate, mistyping_rate) + 
                0.5 * genotyping_error_model(offspring_phenotype, v, u, number_of_alleles, dropout_rate, mistyping_rate); 
              log_fullsib_likelihood += log(offspring_phenotype_probability);
            }
          }
          fullsib_likelihood += exp(log_fullsib_likelihood);
        }
        log_halfsib_likelihood += log(fullsib_likelihood);
      }
      maternal_genotype_posterior.at(genotype) = exp(log_halfsib_likelihood);
      possible_maternal_genotypes.col(genotype) = arma::uvec({w, v});
      genotype++;
    }
  }
  maternal_genotype = 
    possible_maternal_genotypes.col(sample(maternal_genotype_posterior));

  // simulate paternal genotype; 
  // conditionally independent given maternal genotype, after marginalizing over offspring genotypes
  arma::vec paternal_genotype_posterior (number_of_alleles);
  for (auto father : fathers)
  {
    double fullsib_likelihood = 0.;
    arma::uvec offspring_from_father = arma::find(paternity == father);
    for (unsigned u=0; u<number_of_alleles; ++u) // paternal allele
    {
      double paternal_genotype_probability = //1.; //uniform prior
          allele_frequencies_normalized[u]; //hwe prior
      double log_fullsib_likelihood = log(paternal_genotype_probability);
      for (auto offspring : offspring_from_father)
      {
        arma::uvec offspring_phenotype = offspring_phenotypes.col(offspring);
        if (offspring_phenotype.is_finite()) { 
          double offspring_phenotype_probability = // Mendelian segregation probs * phenotype probabilities
            0.5 * genotyping_error_model(offspring_phenotype, maternal_genotype[0], u, number_of_alleles, dropout_rate, mistyping_rate) + 
            0.5 * genotyping_error_model(offspring_phenotype, maternal_genotype[1], u, number_of_alleles, dropout_rate, mistyping_rate); 
          log_fullsib_likelihood += log(offspring_phenotype_probability);
        }
      }
      paternal_genotype_posterior[u] = exp(log_fullsib_likelihood);
    }
    paternal_genotypes.at(father) = sample(paternal_genotype_posterior);
  }

  // simulate offspring genotypes
  for (unsigned sib=0; sib<paternity.n_elem; ++sib)
  {
    arma::uvec offspring_phenotype = offspring_phenotypes.col(sib);
    arma::vec offspring_genotype_posterior (2);
    arma::umat possible_offspring_genotypes (2, 2);
    for (unsigned i=0; i<2; ++i)
    {
      offspring_genotype_posterior[i] = 1.; //Mendelian segregation
      if (offspring_phenotype.is_finite()) { 
        offspring_genotype_posterior[i] *= 
          genotyping_error_model(offspring_phenotype, maternal_genotype[i], 
              paternal_genotypes.at(paternity.at(sib)), number_of_alleles, dropout_rate, mistyping_rate);
      }
      possible_offspring_genotypes.col(i) = 
        arma::uvec({maternal_genotype[i], paternal_genotypes.at(paternity.at(sib))});
    }
    offspring_genotypes.col(sib) = possible_offspring_genotypes.col(sample(offspring_genotype_posterior));
  }

  //simulate numbers of errors given genotypes and phenotypes
  //then new error rates via beta-binomial conjugacy
  unsigned sampled_phenotypes = 0;
  arma::uvec counts_of_errors = {0,0};
  if (maternal_phenotype.is_finite())
  {
    sampled_phenotypes++;
    counts_of_errors += 
      simulate_genotyping_errors(maternal_phenotype, maternal_genotype[0], 
          maternal_genotype[1], number_of_alleles, dropout_rate, mistyping_rate);
  }
  for (unsigned sib=0; sib<paternity.n_elem; ++sib)
  {
    arma::uvec offspring_phenotype = offspring_phenotypes.col(sib);
    if (offspring_phenotype.is_finite()) { 
      sampled_phenotypes++;
      counts_of_errors += 
        simulate_genotyping_errors(offspring_phenotype, offspring_genotypes.at(0,sib), 
          offspring_genotypes.at(1,sib), number_of_alleles, dropout_rate, mistyping_rate);
    }
  }
  double new_dropout_error = 
    0.5 * R::rbeta(1. + counts_of_errors[0], 1. + sampled_phenotypes - counts_of_errors[0]);
  double new_mistyping_error = 
    R::rbeta(1. + counts_of_errors[1], 1. + 2.*sampled_phenotypes - counts_of_errors[1]);

  return arma::vec({new_dropout_error, new_mistyping_error});
}

// [[Rcpp::export]]
Rcpp::List sample_error_rates_given_paternity
 (arma::uvec paternity, 
  arma::ucube offspring_phenotypes, 
  arma::umat maternal_phenotype,
  std::vector<arma::vec> allele_frequencies,
  arma::vec dropout_rate,
  arma::vec mistyping_rate,
  const unsigned max_iter = 1000)
{
  // check number of loci match
  const unsigned number_of_loci = allele_frequencies.size();
  if (maternal_phenotype.n_cols != number_of_loci) Rcpp::stop("must have maternal phenotypes for each locus");
  if (offspring_phenotypes.n_slices != number_of_loci) Rcpp::stop("must have offspring phenotypes for each locus");
  if (dropout_rate.n_elem != number_of_loci) Rcpp::stop("must have dropout rates for each locus");
  if (mistyping_rate.n_elem != number_of_loci) Rcpp::stop("must have mistyping rates for each locus");

  arma::mat dropout_rate_samples (number_of_loci, max_iter);
  arma::mat mistyping_rate_samples (number_of_loci, max_iter);

  for (unsigned iter=0; iter<max_iter; ++iter)
  {
    for (unsigned locus=0; locus<number_of_loci; ++locus)
    {
      arma::vec new_rates =
        sample_error_rates_given_paternity_by_locus(paternity, offspring_phenotypes.slice(locus), 
            maternal_phenotype.col(locus), allele_frequencies[locus], dropout_rate[locus], mistyping_rate[locus]);
      dropout_rate[locus] = new_rates[0];
      mistyping_rate[locus] = new_rates[1];
    }
    dropout_rate_samples.col(iter) = dropout_rate;
    mistyping_rate_samples.col(iter) = mistyping_rate;
    if (iter % 100 == 0) Rcpp::Rcout << "[" << iter << "]" << std::endl;
  }
  return Rcpp::List::create(
      Rcpp::_["dropout_rate"] = dropout_rate_samples,
      Rcpp::_["mistyping_rate"] = mistyping_rate_samples);
}

// [[Rcpp::export]]
double paternity_loglikelihood_by_locus 
 (const arma::uvec& paternity,
  const arma::umat& offspring_phenotypes, 
  const arma::uvec& maternal_phenotype, 
  const arma::vec& allele_frequencies, 
  const double& dropout_rate, 
  const double& mistyping_rate)
{
  // likelihood of offspring paternity given offspring phenotypes, maternal phenotype, haplodiploidy
  // modified from Eqs 3 & 4 in Wang 2004 Genetics
  const unsigned number_of_alleles = allele_frequencies.n_elem;
  const arma::uvec fathers = arma::unique(paternity);
  const arma::vec allele_frequencies_normalized = allele_frequencies / arma::accu(allele_frequencies);
  if (offspring_phenotypes.n_rows != 2) Rcpp::stop("offspring phenotypes must have 2 rows");
  if (offspring_phenotypes.n_cols != paternity.n_elem) Rcpp::stop("offspring phenotypes must have column for each individual");
  if (maternal_phenotype.n_elem != 2) Rcpp::stop("maternal phenotype must have 2 elements");
  if (offspring_phenotypes.max() >= number_of_alleles) Rcpp::stop("offspring allele out of range");
  if (maternal_phenotype.max() >= number_of_alleles) Rcpp::stop("maternal allele out of range");
  if (arma::any(allele_frequencies_normalized < 0.)) Rcpp::stop("negative allele frequencies");
  if (dropout_rate <= 0. || mistyping_rate <= 0.) Rcpp::stop("negative genotyping error rates");
  double halfsib_likelihood = 0.;
  for (unsigned w=0; w<number_of_alleles; ++w) // first maternal allele 
  { 
    for (unsigned v=w; v<number_of_alleles; ++v) // second maternal allele
    {
      double maternal_genotype_probability = //1; //uniform prior
        (2.-int(w==v)) * allele_frequencies_normalized[w] * allele_frequencies_normalized[v]; //hwe prior
      double log_halfsib_likelihood = log(maternal_genotype_probability); 
      if (maternal_phenotype.is_finite())
      {
        double maternal_phenotype_probability = 
          genotyping_error_model(maternal_phenotype, w, v, number_of_alleles, dropout_rate, mistyping_rate);
        log_halfsib_likelihood += log(maternal_phenotype_probability);
      }
      for (auto father : fathers)
      {
        double fullsib_likelihood = 0.;
        arma::uvec offspring_from_father = arma::find(paternity == father);
        for (unsigned u=0; u<number_of_alleles; ++u) // paternal allele
        {
          double paternal_genotype_probability = //1.; //uniform prior
            allele_frequencies_normalized[u]; //hwe prior
          double log_fullsib_likelihood = log(paternal_genotype_probability);
          for (auto offspring : offspring_from_father)
          {
            arma::uvec offspring_phenotype = offspring_phenotypes.col(offspring);
            if (offspring_phenotype.is_finite()) { 
              double offspring_phenotype_probability = // Mendelian segregation probs * phenotype probabilities
                0.5 * genotyping_error_model(offspring_phenotype, w, u, number_of_alleles, dropout_rate, mistyping_rate) + 
                0.5 * genotyping_error_model(offspring_phenotype, v, u, number_of_alleles, dropout_rate, mistyping_rate); 
              log_fullsib_likelihood += log(offspring_phenotype_probability);
            }
          }
          fullsib_likelihood += exp(log_fullsib_likelihood);
        }
        log_halfsib_likelihood += log(fullsib_likelihood);
      }
      halfsib_likelihood += exp(log_halfsib_likelihood);
    }
  }
  return log(halfsib_likelihood);
}

// [[Rcpp::export]]
double paternity_loglikelihood 
 (arma::uvec paternity, 
  arma::ucube offspring_phenotypes, 
  arma::umat maternal_phenotype,
  std::vector<arma::vec> allele_frequencies,
  arma::vec dropout_rate,
  arma::vec mistyping_rate)
{
  // check number of loci match
  const unsigned number_of_loci = allele_frequencies.size();
  if (maternal_phenotype.n_cols != number_of_loci) Rcpp::stop("must have maternal phenotypes for each locus");
  if (offspring_phenotypes.n_slices != number_of_loci) Rcpp::stop("must have offspring phenotypes for each locus");
  if (dropout_rate.n_elem != number_of_loci) Rcpp::stop("must have dropout rates for each locus");
  if (mistyping_rate.n_elem != number_of_loci) Rcpp::stop("must have mistyping rates for each locus");
  double log_likelihood = 0.;
  for (unsigned locus=0; locus<number_of_loci; ++locus)
  {
    log_likelihood += 
      paternity_loglikelihood_by_locus(paternity, offspring_phenotypes.slice(locus), maternal_phenotype.col(locus), allele_frequencies[locus], dropout_rate[locus], mistyping_rate[locus]);
  }
  return log_likelihood;
}

// [[Rcpp::export]]
arma::uvec recode_to_contiguous_integers (arma::uvec input)
{
  arma::uvec uniq = arma::unique(input);
  for(unsigned i=0; i<uniq.n_elem; ++i) input.replace(uniq.at(i), i);
  return input;
}

// [[Rcpp::export]]
Rcpp::List optimize_paternity_given_error_rates
 (arma::uvec paternity, 
  arma::ucube offspring_phenotypes, 
  arma::umat maternal_phenotype,
  std::vector<arma::vec> allele_frequencies,
  arma::vec dropout_rate,
  arma::vec mistyping_rate)
{
  const unsigned max_iter = 1000;
  const double convergence_tolerance = 1e-8;
  unsigned iter;
  double current_loglik = -arma::datum::inf;
  double old_loglik = -arma::datum::inf;
  for (iter=0; iter<=max_iter; ++iter)
  {
    paternity = recode_to_contiguous_integers(paternity);
    old_loglik = current_loglik;
    for (unsigned sib=0; sib<paternity.n_elem; ++sib)
    {
      unsigned current_number_of_fathers = paternity.max() + 1;
      arma::vec log_likelihood (current_number_of_fathers + 1);
      for (unsigned father=0; father<=current_number_of_fathers; ++father)
      {
        paternity[sib] = father;
        log_likelihood[father] = paternity_loglikelihood(paternity, offspring_phenotypes, maternal_phenotype, allele_frequencies, dropout_rate, mistyping_rate);
      }
      paternity[sib] = log_likelihood.index_max();
      paternity = recode_to_contiguous_integers(paternity);
      current_loglik = log_likelihood.max();
      //std::cout << iter << " " << sib << " " << log_likelihood.index_max() << " " << log_likelihood.max() << std::endl;
      //log_likelihood.t().print("loglik");
      //paternity.t().print("paternity");
    }
    Rcpp::Rcout << "[" << iter << "] " << "loglik: " << current_loglik << ", delta: " << current_loglik - old_loglik << std::endl;
    if (current_loglik - old_loglik < convergence_tolerance) break;
  }
  return Rcpp::List::create(
      Rcpp::_["paternity"] = paternity,
      Rcpp::_["loglikelihood"] = current_loglik,
      Rcpp::_["iterations"] = iter,
      Rcpp::_["converged"] = iter < max_iter
      );
}

// [[Rcpp::export]]
Rcpp::List sample_paternity_given_error_rates
 (arma::uvec paternity, 
  arma::ucube offspring_phenotypes, 
  arma::umat maternal_phenotype,
  std::vector<arma::vec> allele_frequencies,
  arma::vec dropout_rate,
  arma::vec mistyping_rate,
  const unsigned max_iter)
{
  // samples from posterior distribution with Dirichlet process prior using algorithm 8 from Neal 2000 JCGS with m = 1
  
  const arma::vec error_rate_bounds = {0., 0.25};
  const unsigned num_loci = allele_frequencies.size();
  const unsigned num_offspring = paternity.n_elem;
  const double alpha = 1.;

  // storage
  arma::umat paternity_samples (num_offspring, max_iter);
  arma::mat dropout_rate_samples (num_loci, max_iter);
  arma::mat mistyping_rate_samples (num_loci, max_iter);
  arma::vec deviance_samples (max_iter);

  double deviance = 0.;
  paternity = recode_to_contiguous_integers(paternity);
  for (unsigned iter=0; iter<max_iter; ++iter)
  {
    // update paternity vector
    for (unsigned sib=0; sib<num_offspring; ++sib)
    {
      // tally size of sib groups
      unsigned current_number_of_fathers = paternity.max() + 1;
      arma::uvec offspring_per_father (current_number_of_fathers + 1, arma::fill::zeros);
      for (auto i : paternity) offspring_per_father[i]++;
      bool sib_is_not_singleton = offspring_per_father[paternity[sib]] > 1;
      offspring_per_father[paternity[sib]]--; 

      // conditional paternity probabilities
      arma::vec log_likelihood (current_number_of_fathers + unsigned(sib_is_not_singleton));
      for (unsigned father=0; father<log_likelihood.n_elem; ++father)
      {
        paternity[sib] = father;
        log_likelihood[father] = paternity_loglikelihood(paternity, offspring_phenotypes, maternal_phenotype, allele_frequencies, dropout_rate, mistyping_rate);
        double log_prior = offspring_per_father[father] > 0 ? log(double(offspring_per_father[father])) : log(alpha);
        log_likelihood[father] += -log(double(paternity.n_elem) - 1. + alpha) + log_prior;
      }

      // sample new father
      Rcpp::IntegerVector possible_fathers (log_likelihood.n_elem);
      Rcpp::NumericVector paternity_probability (log_likelihood.n_elem);
      for (unsigned father=0; father<log_likelihood.n_elem; ++father)
      {
        possible_fathers[father] = father;
        paternity_probability[father] = exp(log_likelihood[father]);
      }
      Rcpp::IntegerVector draw = Rcpp::sample(possible_fathers, 1, false, paternity_probability);
      paternity[sib] = draw(0);
      deviance = -2 * log_likelihood[draw(0)];
      paternity = recode_to_contiguous_integers(paternity); //why? without this it crawls up if preexsiting singleton is rejected
    }

    // update error rates
    // we use data augmention of genotypes:
    //   Pr(maternal|phenotypes) Pr(paternal|maternal,phenotypes) Pr(offspring|maternal,paternal,phenotypes)
    // then use data augmentation of error events. 
    // the update for the error rates is then the usual beta-binomial, except we bound dropout_error at [0, 0.5]

    // store state
    paternity_samples.col(iter) = paternity;
    dropout_rate_samples.col(iter) = dropout_rate;
    mistyping_rate_samples.col(iter) = mistyping_rate;
    deviance_samples.at(iter) = deviance;
    if (iter % 100 == 0) Rcpp::Rcout << "[" << iter << "] " << "deviance: " << deviance << std::endl;
  }
  return Rcpp::List::create(
    Rcpp::_["paternity"] = paternity_samples,
    Rcpp::_["dropout_rate"] = dropout_rate_samples,
    Rcpp::_["mistyping_rate"] = mistyping_rate_samples,
    Rcpp::_["deviance"] = deviance_samples);
}

// optimize_error_rates_given_paternity ==> just spit out gradient, do this in R

