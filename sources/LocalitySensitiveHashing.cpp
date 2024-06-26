/*
 *   Copyright (c) 2004-2005 Massachusetts Institute of Technology.
 *   All Rights Reserved.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *   Authors: Alexandr Andoni (andoni@mit.edu), Piotr Indyk (indyk@mit.edu)
*/

/*
  The main functionality of the LSH scheme is in this file (all except
  the hashing of the buckets). This file includes all the functions
  for processing a PRNearNeighborStructT data structure, which is the
  main R-NN data structure based on LSH scheme. The particular
  functions are: initializing a DS, adding new points to the DS, and
  responding to queries on the DS.
 */

#include "headers.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <queue>
#include <algorithm>
#include <ctime>
#include <vector>

bool is_power_of_two(int n){
    return (n > 0) && ((n & (n-1)) == 0);
}

void printRNNParameters(FILE *output, RNNParametersT parameters){
  ASSERT(output != NULL);
  fprintf(output, "R\n");
  fprintf(output, "%0.9lf\n", parameters.parameterR);
  fprintf(output, "Success probability\n");
  fprintf(output, "%0.9lf\n", parameters.successProbability);
  fprintf(output, "Dimension\n");
  fprintf(output, "%d\n", parameters.dimension);
  fprintf(output, "R^2\n");
  fprintf(output, "%0.9lf\n", parameters.parameterR2);
  fprintf(output, "Use <u> functions\n");
  fprintf(output, "%d\n", parameters.useUfunctions);
  fprintf(output, "k\n");
  fprintf(output, "%d\n", parameters.parameterK);
  fprintf(output, "m [# independent tuples of LSH functions]\n");
  fprintf(output, "%d\n", parameters.parameterM);
  fprintf(output, "L\n");
  fprintf(output, "%d\n", parameters.parameterL);
  fprintf(output, "W\n");
  fprintf(output, "%0.9lf\n", parameters.parameterW);
  fprintf(output, "T\n");
  fprintf(output, "%d\n", parameters.parameterT);
  fprintf(output, "typeHT\n");
  fprintf(output, "%d\n", parameters.typeHT);
}

RNNParametersT readRNNParameters(FILE *input){
  ASSERT(input != NULL);
  RNNParametersT parameters;
  char s[1000];// TODO: possible buffer overflow

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.parameterR);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.successProbability);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.dimension);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.parameterR2);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.useUfunctions);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterK);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterM);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterL);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  FSCANF_REAL(input, &parameters.parameterW);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.parameterT);

  fscanf(input, "\n");fscanf(input, "%[^\n]\n", s);
  fscanf(input, "%d", &parameters.typeHT);

  return parameters;
}

// Creates the LSH hash functions for the R-near neighbor structure
// <nnStruct>. The functions fills in the corresponding field of
// <nnStruct>.
void initHashFunctions(PRNearNeighborStructT nnStruct){
  ASSERT(nnStruct != NULL);
  LSHFunctionT **lshFunctions;
  // allocate memory for the functions
  FAILIF(NULL == (lshFunctions = (LSHFunctionT**)MALLOC(nnStruct->nHFTuples * sizeof(LSHFunctionT*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (lshFunctions[i] = (LSHFunctionT*)MALLOC(nnStruct->hfTuplesLength * sizeof(LSHFunctionT))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (lshFunctions[i][j].a = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
    }
  }

  // initialize the LSH functions
  srand((unsigned)(time(0)));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    //printf("hash table is %d\n", i);
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      //printf("k is %d\n", j);
      // vector a
      for(IntT d = 0; d < nnStruct->dimension; d++){
#ifdef USE_L1_DISTANCE
	lshFunctions[i][j].a[d] = genCauchyRandom();
#else
  
	//lshFunctions[i][j].a[d] = FgenGaussianRandom(nnStruct->dimension);
  //lshFunctions[i][j].a[d] = FFgenGaussianRandom(nnStruct->dimension);
  lshFunctions[i][j].a[d] = genGaussianRandom();
  //printf("%f ", lshFunctions[i][j].a[d]);
#endif
      }
      // b
      lshFunctions[i][j].b = genUniformRandom(0, nnStruct->parameterW);
    }
  }

  nnStruct->lshFunctions = lshFunctions;

  randomdim **ran_dim;
  // allocate memory for the functions
  FAILIF(NULL == (ran_dim = (randomdim**)MALLOC(nnStruct->nHFTuples * sizeof(randomdim*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (ran_dim[i] = (randomdim*)MALLOC(nnStruct->hfTuplesLength * sizeof(randomdim))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (ran_dim[i][j].c = (int*)MALLOC(nnStruct->dimension * sizeof(int))));
    }
  }

  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      std::vector<int> dim;
      dim.resize(nnStruct->dimension);
      for (size_t k = 0; k < nnStruct->dimension; ++k){
        dim.at(k) = k;
      }
      std::random_shuffle(dim.begin(), dim.end());
      for(IntT d = 0; d < nnStruct->dimension; d++){
        IntT Cdim = dim[d];
        ran_dim[i][j].c[d] = Cdim;
        //printf("%d ", ran_dim[i][j].c[d]);
      }
      //printf("\n");
    }
  }
  nnStruct->ran_dim = ran_dim;

  randomdim **diagonal;
  // allocate memory for the functions
  FAILIF(NULL == (diagonal = (randomdim**)MALLOC(nnStruct->nHFTuples * sizeof(randomdim*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (diagonal[i] = (randomdim*)MALLOC(nnStruct->hfTuplesLength * sizeof(randomdim))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (diagonal[i][j].c = (int*)MALLOC(nnStruct->dimension * sizeof(int))));
    }
  }

  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      CreateDiagonal(nnStruct->dimension, diagonal[i][j].c);
      // for(IntT d = 0; d < nnStruct->dimension; d++){
      //   printf("%d ", diagonal[i][j].c[d]);
      // }
    }
  }
  nnStruct->diagonal = diagonal;

  //std::cout<<"000"<<std::endl;

}

void FinitHashFunctions(PRNearNeighborStructT nnStruct){
  ASSERT(nnStruct != NULL);
  LSHFunctionT **lshFunctions;
  // allocate memory for the functions
  FAILIF(NULL == (lshFunctions = (LSHFunctionT**)MALLOC(nnStruct->nHFTuples * sizeof(LSHFunctionT*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (lshFunctions[i] = (LSHFunctionT*)MALLOC(nnStruct->hfTuplesLength * sizeof(LSHFunctionT))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (lshFunctions[i][j].a = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
    }
  }

  // initialize the LSH functions
  srand((unsigned)(time(0)));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      for(IntT d = 0; d < nnStruct->dimension; d++){
#ifdef USE_L1_DISTANCE
	lshFunctions[i][j].a[d] = genCauchyRandom();
#else
  
	//lshFunctions[i][j].a[d] = FgenGaussianRandom(nnStruct->dimension);
  lshFunctions[i][j].a[d] = FFgenGaussianRandom(nnStruct->dimension);
  //lshFunctions[i][j].a[d] = genGaussianRandom();
  // printf("%f ", lshFunctions[i][j].a[d]);
#endif
      }
      // b
      lshFunctions[i][j].b = genUniformRandom(0, nnStruct->parameterW);
    }
  }

  nnStruct->lshFunctions = lshFunctions;

  randomdim **ran_dim;
  // allocate memory for the functions
  FAILIF(NULL == (ran_dim = (randomdim**)MALLOC(nnStruct->nHFTuples * sizeof(randomdim*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (ran_dim[i] = (randomdim*)MALLOC(nnStruct->hfTuplesLength * sizeof(randomdim))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (ran_dim[i][j].c = (int*)MALLOC(nnStruct->dimension * sizeof(int))));
    }
  }

  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      std::vector<int> dim;
      dim.resize(nnStruct->dimension);
      for (size_t k = 0; k < nnStruct->dimension; ++k){
        dim.at(k) = k;
      }
      std::random_shuffle(dim.begin(), dim.end());
      for(IntT d = 0; d < nnStruct->dimension; d++){
        IntT Cdim = dim[d];
        ran_dim[i][j].c[d] = Cdim;
      }
    }
  }
  nnStruct->ran_dim = ran_dim;

  randomdim **diagonal;
  // allocate memory for the functions
  FAILIF(NULL == (diagonal = (randomdim**)MALLOC(nnStruct->nHFTuples * sizeof(randomdim*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (diagonal[i] = (randomdim*)MALLOC(nnStruct->hfTuplesLength * sizeof(randomdim))));
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      FAILIF(NULL == (diagonal[i][j].c = (int*)MALLOC(nnStruct->dimension * sizeof(int))));
    }
  }

  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
      CreateDiagonal(nnStruct->dimension, diagonal[i][j].c);
    }
  }
  nnStruct->diagonal = diagonal;

}

// Initializes the fields of a R-near neighbors data structure except
// the hash tables for storing the buckets.
PRNearNeighborStructT initializePRNearNeighborFields(RNNParametersT algParameters, Int32T nPointsEstimate){
  PRNearNeighborStructT nnStruct;
  FAILIF(NULL == (nnStruct = (PRNearNeighborStructT)MALLOC(sizeof(RNearNeighborStructT))));
  nnStruct->parameterR = algParameters.parameterR;
  nnStruct->parameterR2 = algParameters.parameterR2;
  nnStruct->useUfunctions = algParameters.useUfunctions;
  nnStruct->parameterK = algParameters.parameterK;
  if (!algParameters.useUfunctions) {
    // Use normal <g> functions.
    nnStruct->parameterL = algParameters.parameterL;
    nnStruct->nHFTuples = algParameters.parameterL;
    nnStruct->hfTuplesLength = algParameters.parameterK;
  }else{
    // Use <u> hash functions; a <g> function is a pair of 2 <u> functions.
    nnStruct->parameterL = algParameters.parameterL;
    nnStruct->nHFTuples = algParameters.parameterM;
    nnStruct->hfTuplesLength = algParameters.parameterK / 2;
  }
  nnStruct->parameterT = algParameters.parameterT;
  nnStruct->dimension = algParameters.dimension;
  nnStruct->parameterW = algParameters.parameterW;

  nnStruct->nPoints = 0;
  nnStruct->pointsArraySize = nPointsEstimate;

  FAILIF(NULL == (nnStruct->points = (PPointT*)MALLOC(nnStruct->pointsArraySize * sizeof(PPointT))));

  // create the hash functions
  FinitHashFunctions(nnStruct);

  // init fields that are used only in operations ("temporary" variables for operations).

  // init the vector <pointULSHVectors> and the vector
  // <precomputedHashesOfULSHs>
  FAILIF(NULL == (nnStruct->pointULSHVectors = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (nnStruct->pointULSHVectors[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
  }

  FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs = (Uns32T**)MALLOC(nnStruct->nHFTuples * sizeof(Uns32T*))));
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FAILIF(NULL == (nnStruct->precomputedHashesOfULSHs[i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
  }

  // init the vector <reducedPoint>
  FAILIF(NULL == (nnStruct->reducedPoint = (RealT*)MALLOC(nnStruct->dimension * sizeof(RealT))));
  // init the vector <nearPoints>
  nnStruct->sizeMarkedPoints = nPointsEstimate;
  FAILIF(NULL == (nnStruct->markedPoints = (BooleanT*)MALLOC(nnStruct->sizeMarkedPoints * sizeof(BooleanT))));
  for(IntT i = 0; i < nnStruct->sizeMarkedPoints; i++){
    nnStruct->markedPoints[i] = FALSE;
  }
  // init the vector <nearPointsIndeces>
  FAILIF(NULL == (nnStruct->markedPointsIndeces = (Int32T*)MALLOC(nnStruct->sizeMarkedPoints * sizeof(Int32T))));

  nnStruct->reportingResult = TRUE;

  return nnStruct;
}

// Constructs a new empty R-near-neighbor data structure.
PRNearNeighborStructT initLSH(RNNParametersT algParameters, Int32T nPointsEstimate){
  ASSERT(algParameters.typeHT == HT_LINKED_LIST || algParameters.typeHT == HT_STATISTICS);
  PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(algParameters, nPointsEstimate);

  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  Uns32T *mainHashA = NULL, *controlHash1 = NULL;
  BooleanT uhashesComputedAlready = FALSE;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPointsEstimate, nnStruct->parameterK, uhashesComputedAlready, mainHashA, controlHash1, NULL);
    uhashesComputedAlready = TRUE;
  }

  return nnStruct;
}

void preparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point);

void preparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point, int subdim);
void first_hadamard_transform(double* work, int N, double* output); 
void second_hadamard_transform(double* work, int N, double* output); 
void FpreparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, double* point, int subdim);


// Construct PRNearNeighborStructT given the data set <dataSet> (all
// the points <dataSet> will be contained in the resulting DS).
// Currenly only type HT_HYBRID_CHAINS is supported for this
// operation.
PRNearNeighborStructT initLSH_WithDataSet(RNNParametersT algParameters, Int32T nPoints, PPointT *dataSet){
  ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(algParameters.typeHT == HT_LINKED_LIST);

  ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

  PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(algParameters, nPoints);

  // Set the fields <nPoints> and <points>.
  nnStruct->nPoints = nPoints;
  for(Int32T i = 0; i < nPoints; i++){
    nnStruct->points[i] = dataSet[i];
  }
  
  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  Uns32T *mainHashA = NULL, *controlHash1 = NULL;
  PUHashStructureT modelHT = newUHashStructure(HT_LINKED_LIST, nPoints, nnStruct->parameterK, FALSE, mainHashA, controlHash1, NULL);
  
  Uns32T **(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T**)MALLOC(nPoints * sizeof(Uns32T*))));
    for(IntT i = 0; i < nPoints; i++){
      FAILIF(NULL == (precomputedHashesOfULSHs[l][i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
    }
  }
  

  clock_t start, end;
  start = clock();
  for(IntT i = 0; i < nPoints; i++){

    preparePointAdding(nnStruct, modelHT, dataSet[i]);

    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	      precomputedHashesOfULSHs[l][i][h] = nnStruct->precomputedHashesOfULSHs[l][h];
      }
    }
  }
  end = clock();
  std::cout<<"time of computing hash value is "<<(double)(end-start) / CLOCKS_PER_SEC <<"(s)"<<std::endl;
  //DPRINTF("Allocated memory(modelHT and precomputedHashesOfULSHs just a.): %lld\n", totalAllocatedMemory);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    // build the model HT.
    for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	      // Use usual <g> functions (truly independent; <g>s are precisly
	      // <u>s).
	      addBucketEntry(modelHT, 1, precomputedHashesOfULSHs[i][p], NULL, p);
        //std::cout<<"list"<<' ';
      } else {
	      // Use <u> functions (<g>s are pairs of <u> functions).
	      addBucketEntry(modelHT, 2, precomputedHashesOfULSHs[firstUComp][p], precomputedHashesOfULSHs[secondUComp][p], p);
      }
    }

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.

    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }

    // copy the model HT into the actual (packed) HT. copy the uhash function too.

    nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

    // clear the model HT for the next iteration.
    clearUHashStructure(modelHT);
  }

  freeUHashStructure(modelHT, FALSE); // do not free the uhash functions since they are used by nnStruct->hashedBuckets[i]

  // freeing precomputedHashesOfULSHs
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT i = 0; i < nPoints; i++){
      FREE(precomputedHashesOfULSHs[l][i]);
    }
    FREE(precomputedHashesOfULSHs[l]);
  }

  return nnStruct;
}


PRNearNeighborStructT FinitLSH_WithDataSet(RNNParametersT algParameters, Int32T nPoints, PPointT *dataSet, int subdim){
  ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);

  
  ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

  PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(algParameters, nPoints);

  // Set the fields <nPoints> and <points>.
  nnStruct->nPoints = nPoints;
  for(Int32T i = 0; i < nPoints; i++){
    nnStruct->points[i] = dataSet[i];
  }
  
  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  Uns32T *mainHashA = NULL, *controlHash1 = NULL;
  PUHashStructureT modelHT = newUHashStructure(HT_LINKED_LIST, nPoints, nnStruct->parameterK, FALSE, mainHashA, controlHash1, NULL);
  
  Uns32T **(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T**)MALLOC(nPoints * sizeof(Uns32T*))));
    for(IntT i = 0; i < nPoints; i++){
      FAILIF(NULL == (precomputedHashesOfULSHs[l][i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
    }
  }

  clock_t start, end;
  start = clock();

  
  for(IntT i = 0; i < nPoints; i++){

    //*********************************
    //  ACHash
    //*********************************
    if(is_power_of_two(nnStruct->dimension)){
      double firstHT[nnStruct->dimension];
      double *temp = dataSet[i]->coordinates;
      first_hadamard_transform(temp, nnStruct->dimension, firstHT);
      FpreparePointAdding(nnStruct, modelHT, firstHT, subdim);
    }else{
      int dimension = pow(2, ceil(log2(nnStruct->dimension)));
      double firstHT[dimension];
      double temp[dimension] = {0}; 
      for (IntT d = 0; d < nnStruct->dimension; ++d){temp[d] = dataSet[i]->coordinates[d];}
      first_hadamard_transform(temp, dimension, firstHT);
      FpreparePointAdding(nnStruct, modelHT, firstHT, subdim);
    }
    

    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	      precomputedHashesOfULSHs[l][i][h] = nnStruct->precomputedHashesOfULSHs[l][h];
      }
    }
  }

  end = clock();
  std::cout<<"time of computing hash value is "<<(double)(end-start) / CLOCKS_PER_SEC <<"(s)"<<std::endl;

  //DPRINTF("Allocated memory(modelHT and precomputedHashesOfULSHs just a.): %lld\n", totalAllocatedMemory);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    // build the model HT.
    for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	      // Use usual <g> functions (truly independent; <g>s are precisly
	      // <u>s).
	      addBucketEntry(modelHT, 1, precomputedHashesOfULSHs[i][p], NULL, p);
      } else {
	      // Use <u> functions (<g>s are pairs of <u> functions).
	      addBucketEntry(modelHT, 2, precomputedHashesOfULSHs[firstUComp][p], precomputedHashesOfULSHs[secondUComp][p], p);
      }
    }

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }
    // clock_t one, two;
    // one = clock();
    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);
    // two = clock();
    // std::cout<<"time is "<<(double)(two-one) / CLOCKS_PER_SEC <<"(s)"<<std::endl;

    // clear the model HT for the next iteration.
    clearUHashStructure(modelHT);
  }

  freeUHashStructure(modelHT, FALSE); // do not free the uhash functions since they are used by nnStruct->hashedBuckets[i]

  // freeing precomputedHashesOfULSHs
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT i = 0; i < nPoints; i++){
      FREE(precomputedHashesOfULSHs[l][i]);
    }
    FREE(precomputedHashesOfULSHs[l]);
  }

  return nnStruct;
}


PRNearNeighborStructT RinitLSH_WithDataSet(RNNParametersT algParameters, Int32T nPoints, PPointT *dataSet, int subdim){
  ASSERT(algParameters.typeHT == HT_HYBRID_CHAINS);
  //ASSERT(algParameters.typeHT == HT_LINKED_LIST);

  
  // std::cout<<"begin build index"<<std::endl;
  ASSERT(dataSet != NULL);
  ASSERT(USE_SAME_UHASH_FUNCTIONS);

  PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(algParameters, nPoints);

  // Set the fields <nPoints> and <points>.
  nnStruct->nPoints = nPoints;
  for(Int32T i = 0; i < nPoints; i++){
    nnStruct->points[i] = dataSet[i];
  }
  
  // initialize second level hashing (bucket hashing)
  FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
  Uns32T *mainHashA = NULL, *controlHash1 = NULL;
  PUHashStructureT modelHT = newUHashStructure(HT_LINKED_LIST, nPoints, nnStruct->parameterK, FALSE, mainHashA, controlHash1, NULL);
  
  Uns32T **(precomputedHashesOfULSHs[nnStruct->nHFTuples]);
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    FAILIF(NULL == (precomputedHashesOfULSHs[l] = (Uns32T**)MALLOC(nPoints * sizeof(Uns32T*))));
    for(IntT i = 0; i < nPoints; i++){
      //FAILIF(NULL == (precomputedHashesOfULSHs[l][i] = (Uns32T*)MALLOC(N_PRECOMPUTED_HASHES_NEEDED * sizeof(Uns32T))));
      FAILIF(NULL == (precomputedHashesOfULSHs[l][i] = (Uns32T*)MALLOC(nnStruct->hfTuplesLength * sizeof(Uns32T))));
    }
  }

  clock_t start, end;
  start = clock();

  for(IntT i = 0; i < nPoints; i++){

    RpreparePointAdding(nnStruct, modelHT, dataSet[i], subdim);

    for(IntT l = 0; l < nnStruct->nHFTuples; l++){
      for(IntT h = 0; h < N_PRECOMPUTED_HASHES_NEEDED; h++){
	      precomputedHashesOfULSHs[l][i][h] = nnStruct->precomputedHashesOfULSHs[l][h];
      }
    }
  }

  end = clock();
  std::cout<<"time of computing hash value is "<<(double)(end-start) / CLOCKS_PER_SEC <<"(s)"<<std::endl;

  //DPRINTF("Allocated memory(modelHT and precomputedHashesOfULSHs just a.): %lld\n", totalAllocatedMemory);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    // build the model HT.
    for(IntT p = 0; p < nPoints; p++){
      // Add point <dataSet[p]> to modelHT.
      if (!nnStruct->useUfunctions) {
	      // Use usual <g> functions (truly independent; <g>s are precisly
	      // <u>s).
	      addBucketEntry(modelHT, 1, precomputedHashesOfULSHs[i][p], NULL, p);
      } else {
	      // Use <u> functions (<g>s are pairs of <u> functions).
	      addBucketEntry(modelHT, 2, precomputedHashesOfULSHs[firstUComp][p], precomputedHashesOfULSHs[secondUComp][p], p);
      }
    }

    //ASSERT(nAllocatedGBuckets <= nPoints);
    //ASSERT(nAllocatedBEntries <= nPoints);

    // compute what is the next pair of <u> functions.
    secondUComp++;
    if (secondUComp == nnStruct->nHFTuples) {
      firstUComp++;
      secondUComp = firstUComp + 1;
    }
    // clock_t one, two;
    // one = clock();
    // copy the model HT into the actual (packed) HT. copy the uhash function too.
    nnStruct->hashedBuckets[i] = newUHashStructure(algParameters.typeHT, nPoints, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);
    // two = clock();
    // std::cout<<"time is "<<(double)(two-one) / CLOCKS_PER_SEC <<"(s)"<<std::endl;

    // clear the model HT for the next iteration.
    clearUHashStructure(modelHT);
  }

  freeUHashStructure(modelHT, FALSE); // do not free the uhash functions since they are used by nnStruct->hashedBuckets[i]

  // freeing precomputedHashesOfULSHs
  for(IntT l = 0; l < nnStruct->nHFTuples; l++){
    for(IntT i = 0; i < nPoints; i++){
      FREE(precomputedHashesOfULSHs[l][i]);
    }
    FREE(precomputedHashesOfULSHs[l]);
  }

  return nnStruct;
}


// // Packed version (static).
// PRNearNeighborStructT buildPackedLSH(RealT R, BooleanT useUfunctions, IntT k, IntT LorM, RealT successProbability, IntT dim, IntT T, Int32T nPoints, PPointT *points){
//   ASSERT(points != NULL);
//   PRNearNeighborStructT nnStruct = initializePRNearNeighborFields(R, useUfunctions, k, LorM, successProbability, dim, T, nPoints);

//   // initialize second level hashing (bucket hashing)
//   FAILIF(NULL == (nnStruct->hashedBuckets = (PUHashStructureT*)MALLOC(nnStruct->parameterL * sizeof(PUHashStructureT))));
//   Uns32T *mainHashA = NULL, *controlHash1 = NULL;
//   PUHashStructureT modelHT = newUHashStructure(HT_STATISTICS, nPoints, nnStruct->parameterK, FALSE, mainHashA, controlHash1, NULL);
//   for(IntT i = 0; i < nnStruct->parameterL; i++){
//     // build the model HT.
//     for(IntT p = 0; p < nPoints; p++){
//       // addBucketEntry(modelHT, );
//     }



//     // copy the model HT into the actual (packed) HT.
//     nnStruct->hashedBuckets[i] = newUHashStructure(HT_PACKED, nPointsEstimate, nnStruct->parameterK, TRUE, mainHashA, controlHash1, modelHT);

//     // clear the model HT for the next iteration.
//     clearUHashStructure(modelHT);
//   }

//   return nnStruct;
// }


// Optimizes the nnStruct (non-agressively, i.e., without changing the
// parameters).
void optimizeLSH(PRNearNeighborStructT nnStruct){
  ASSERT(nnStruct != NULL);

  PointsListEntryT *auxList = NULL;
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    optimizeUHashStructure(nnStruct->hashedBuckets[i], auxList);
  }
  FREE(auxList);
}

// Frees completely all the memory occupied by the <nnStruct>
// structure.
void freePRNearNeighborStruct(PRNearNeighborStructT nnStruct){
  if (nnStruct == NULL){
    return;
  }

  if (nnStruct->points != NULL) {
    free(nnStruct->points);
  }
  
  if (nnStruct->lshFunctions != NULL) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      for(IntT j = 0; j < nnStruct->hfTuplesLength; j++){
	free(nnStruct->lshFunctions[i][j].a);
      }
      free(nnStruct->lshFunctions[i]);
    }
    free(nnStruct->lshFunctions);
  }
  
  if (nnStruct->precomputedHashesOfULSHs != NULL) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      free(nnStruct->precomputedHashesOfULSHs[i]);
    }
    free(nnStruct->precomputedHashesOfULSHs);
  }

  freeUHashStructure(nnStruct->hashedBuckets[0], TRUE);
  for(IntT i = 1; i < nnStruct->parameterL; i++){
    freeUHashStructure(nnStruct->hashedBuckets[i], FALSE);
  }
  free(nnStruct->hashedBuckets);

  if (nnStruct->pointULSHVectors != NULL){
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      free(nnStruct->pointULSHVectors[i]);
    }
    free(nnStruct->pointULSHVectors);
  }

  if (nnStruct->reducedPoint != NULL){
    free(nnStruct->reducedPoint);
  }

  if (nnStruct->markedPoints != NULL){
    free(nnStruct->markedPoints);
  }

  if (nnStruct->markedPointsIndeces != NULL){
    free(nnStruct->markedPointsIndeces);
  }
}

// If <reportingResult> == FALSe, no points are reported back in a
// <get> function. In particular any point that is found in the bucket
// is considered to be outside the R-ball of the query point.  If
// <reportingResult> == TRUE, then the structure behaves normally.
void setResultReporting(PRNearNeighborStructT nnStruct, BooleanT reportingResult){
  ASSERT(nnStruct != NULL);
  nnStruct->reportingResult = reportingResult;
}

// Compute the value of a hash function u=lshFunctions[gNumber] (a
// vector of <hfTuplesLength> LSH functions) in the point <point>. The
// result is stored in the vector <vectorValue>. <vectorValue> must be
// already allocated (and have space for <hfTuplesLength> Uns32T-words).
inline void computeULSH(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, Uns32T *vectorValue){
  CR_ASSERT(nnStruct != NULL);
  CR_ASSERT(point != NULL);
  CR_ASSERT(vectorValue != NULL);

  for(IntT i = 0; i < nnStruct->hfTuplesLength; i++){
    RealT value = 0;

    for(IntT d = 0; d < nnStruct->dimension; d++){
      value += point[d] * nnStruct->lshFunctions[gNumber][i].a[d];
    }
  
    vectorValue[i] = (Uns32T)(FLOOR_INT32((value + nnStruct->lshFunctions[gNumber][i].b) / nnStruct->parameterW) /* - MIN_INT32T*/);
  }
}

inline void FcomputeULSH(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, Uns32T *vectorValue, int subdim){
  CR_ASSERT(nnStruct != NULL);
  CR_ASSERT(point != NULL);
  CR_ASSERT(vectorValue != NULL);
  

  for(IntT i = 0; i < nnStruct->hfTuplesLength; i++){
    RealT value = 0;

    for(IntT d = 0; d < subdim; d++){
      int dim = nnStruct->ran_dim[gNumber][i].c[d];
      value += point[dim] * nnStruct->lshFunctions[gNumber][i].a[d];
    }
    vectorValue[i] = (Uns32T)(FLOOR_INT32((value + nnStruct->lshFunctions[gNumber][i].b) / nnStruct->parameterW));
  }
}

inline void computeULSH(PRNearNeighborStructT nnStruct, IntT gNumber, RealT *point, Uns32T *vectorValue, int subdim){
  CR_ASSERT(nnStruct != NULL);
  CR_ASSERT(point != NULL);
  CR_ASSERT(vectorValue != NULL);
  

  for(IntT i = 0; i < nnStruct->hfTuplesLength; i++){
    RealT value = 0;

    for(IntT d = 0; d < subdim; d++){
      int dim = nnStruct->ran_dim[gNumber][i].c[d];
      value += point[dim] * nnStruct->lshFunctions[gNumber][i].a[d];
    } 
    vectorValue[i] = (Uns32T)(FLOOR_INT32((value + nnStruct->lshFunctions[gNumber][i].b) / nnStruct->parameterW) /* - MIN_INT32T*/);
  }
} 




void first_hadamard_transform(double* work, int N, double* output) 
{
	int i, j, k, stage, L, J, M;
	double *tmp;
	for (i = 0; i < N-1; i += 2)
	{
		work[i] = (work[i] + work[i+1]);
		work[i+1] = (work[i] - 2*work[i+1]);
	}
	L = 1;
	for (stage = 2; stage <= ceil(log2(N)); ++stage)
	{
		M = (int) pow((float)2, L);
		J = 0;
		k = 0;

		while (k < N-1)
		{
			for (j = J; j < J+M-1; j = j+2)
			{
				output[k] = work[j] + work[j+M];
				output[k+1] = work[j] - work[j+M];
				output[k+2] = work[j+1] - work[j+M+1];
				output[k+3] = work[j+1] + work[j+M+1];
				k += 4;
			}
			J += 2*M;
		}
		tmp = work;
		work = output;
		output = tmp;
		L += 1;
	}
}

void second_hadamard_transform(double* work, int N, double* output) 
{
	int i, j, k, stage, L, J, M;
	double *tmp;
	for (i = 0; i < N-1; i += 2)
	{
		work[i] = (work[i] + work[i+1]);
		work[i+1] = (work[i] - 2*work[i+1]);
	}
	L = 1;
	for (stage = 2; stage <= ceil(log2(N)); ++stage)
	{
		M = (int) pow((float)2, L);
		J = 0;
		k = 0;

		while (k < N-1)
		{
			for (j = J; j < J+M-1; j = j+2)
			{
				output[k] = work[j] + work[j+M];
				output[k+1] = work[j] - work[j+M];
				output[k+2] = work[j+1] - work[j+M+1];
				output[k+3] = work[j+1] + work[j+M+1];
				k += 4;
			}
			J += 2*M;
		}
		tmp = work;
		work = output;
		output = tmp;
		L += 1;
	}
}

inline void preparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point){
  ASSERT(nnStruct != NULL);
  ASSERT(uhash != NULL);
  ASSERT(point != NULL);

  TIMEV_START(timeComputeULSH);



  for(IntT d = 0; d < nnStruct->dimension; d++){
    nnStruct->reducedPoint[d] = point->coordinates[d] /*/ nnStruct->parameterR*/;
  }

  // Compute all ULSH functions.
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    computeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i]);
  }

  // Compute data for <precomputedHashesOfULSHs>.
  if (USE_SAME_UHASH_FUNCTIONS) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      precomputeUHFsForULSH(uhash, nnStruct->pointULSHVectors[i], nnStruct->hfTuplesLength, nnStruct->precomputedHashesOfULSHs[i]);
    }
  }

  TIMEV_END(timeComputeULSH);
}

  inline void FpreparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, double* point, int subdim){
  ASSERT(nnStruct != NULL);
  ASSERT(uhash != NULL);
  ASSERT(point != NULL);

  TIMEV_START(timeComputeULSH);

  for(IntT d = 0; d < nnStruct->dimension; d++){
    nnStruct->reducedPoint[d] = point[d] /*/ nnStruct->parameterR*/;
  }

  // ACHash
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    FcomputeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i], subdim);
  }

  // Compute data for <precomputedHashesOfULSHs>.
  if (USE_SAME_UHASH_FUNCTIONS) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      precomputeUHFsForULSH(uhash, nnStruct->pointULSHVectors[i], nnStruct->hfTuplesLength, nnStruct->precomputedHashesOfULSHs[i]);
    }
  }

  TIMEV_END(timeComputeULSH);
}

void RpreparePointAdding(PRNearNeighborStructT nnStruct, PUHashStructureT uhash, PPointT point, int subdim){
  ASSERT(nnStruct != NULL);
  ASSERT(uhash != NULL);
  ASSERT(point != NULL);

  TIMEV_START(timeComputeULSH);


  for(IntT d = 0; d < nnStruct->dimension; d++){
    nnStruct->reducedPoint[d] = point->coordinates[d] /*/ nnStruct->parameterR*/;
  }
  

  // Compute all ULSH functions.
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    computeULSH(nnStruct, i, nnStruct->reducedPoint, nnStruct->pointULSHVectors[i], subdim);
  }

  // Compute data for <precomputedHashesOfULSHs>.
  if (USE_SAME_UHASH_FUNCTIONS) {
    for(IntT i = 0; i < nnStruct->nHFTuples; i++){
      precomputeUHFsForULSH(uhash, nnStruct->pointULSHVectors[i], nnStruct->hfTuplesLength, nnStruct->precomputedHashesOfULSHs[i]);
      
    }
  }

  TIMEV_END(timeComputeULSH);
}

inline void batchAddRequest(PRNearNeighborStructT nnStruct, IntT i, IntT &firstIndex, IntT &secondIndex, PPointT point){
//   Uns32T *(gVector[4]);
//   if (!nnStruct->useUfunctions) {
//     // Use usual <g> functions (truly independent).
//     gVector[0] = nnStruct->pointULSHVectors[i];
//     gVector[1] = nnStruct->precomputedHashesOfULSHs[i];
//     addBucketEntry(nnStruct->hashedBuckets[firstIndex], gVector, 1, point);
//   } else {
//     // Use <u> functions (<g>s are pairs of <u> functions).
//     gVector[0] = nnStruct->pointULSHVectors[firstIndex];
//     gVector[1] = nnStruct->pointULSHVectors[secondIndex];
//     gVector[2] = nnStruct->precomputedHashesOfULSHs[firstIndex];
//     gVector[3] = nnStruct->precomputedHashesOfULSHs[secondIndex];
    
//     // compute what is the next pair of <u> functions.
//     secondIndex++;
//     if (secondIndex == nnStruct->nHFTuples) {
//       firstIndex++;
//       secondIndex = firstIndex + 1;
//     }
    
//     addBucketEntry(nnStruct->hashedBuckets[i], gVector, 2, point);
//   }
  ASSERT(1 == 0);
}

// Adds a new point to the LSH data structure, that is for each
// i=0..parameterL-1, the point is added to the bucket defined by
// function g_i=lshFunctions[i].
void addNewPointToPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT point){
  ASSERT(nnStruct != NULL);
  ASSERT(point != NULL);
  ASSERT(nnStruct->reducedPoint != NULL);
  ASSERT(!nnStruct->useUfunctions || nnStruct->pointULSHVectors != NULL);
  ASSERT(nnStruct->hashedBuckets[0]->typeHT == HT_LINKED_LIST || nnStruct->hashedBuckets[0]->typeHT == HT_STATISTICS);

  nnStruct->points[nnStruct->nPoints] = point;
  nnStruct->nPoints++;

  preparePointAdding(nnStruct, nnStruct->hashedBuckets[0], point);

  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;

  TIMEV_START(timeBucketIntoUH);
  for(IntT i = 0; i < nnStruct->parameterL; i++){
    if (!nnStruct->useUfunctions) {
      // Use usual <g> functions (truly independent; <g>s are precisly
      // <u>s).
      addBucketEntry(nnStruct->hashedBuckets[i], 1, nnStruct->precomputedHashesOfULSHs[i], NULL, nnStruct->nPoints - 1);
    } else {
      // Use <u> functions (<g>s are pairs of <u> functions).
      addBucketEntry(nnStruct->hashedBuckets[i], 2, nnStruct->precomputedHashesOfULSHs[firstUComp], nnStruct->precomputedHashesOfULSHs[secondUComp], nnStruct->nPoints - 1);

      // compute what is the next pair of <u> functions.
      secondUComp++;
      if (secondUComp == nnStruct->nHFTuples) {
	      firstUComp++;
	      secondUComp = firstUComp + 1;
      }
    }
    //batchAddRequest(nnStruct, i, firstUComp, secondUComp, point);
  }
  TIMEV_END(timeBucketIntoUH);

  // Check whether the vectors <nearPoints> & <nearPointsIndeces> is still big enough.
  if (nnStruct->nPoints > nnStruct->sizeMarkedPoints) {
    nnStruct->sizeMarkedPoints = 2 * nnStruct->nPoints;
    FAILIF(NULL == (nnStruct->markedPoints = (BooleanT*)REALLOC(nnStruct->markedPoints, nnStruct->sizeMarkedPoints * sizeof(BooleanT))));
    for(IntT i = 0; i < nnStruct->sizeMarkedPoints; i++){
      nnStruct->markedPoints[i] = FALSE;
    }
    FAILIF(NULL == (nnStruct->markedPointsIndeces = (Int32T*)REALLOC(nnStruct->markedPointsIndeces, nnStruct->sizeMarkedPoints * sizeof(Int32T))));
  }
}

// Returns TRUE iff |p1-p2|_2^2 <= threshold
inline BooleanT isDistanceSqrLeq(IntT dimension, PPointT p1, PPointT p2, RealT threshold){
  RealT result = 0;
  nOfDistComps++;

  TIMEV_START(timeDistanceComputation);
  for (IntT i = 0; i < dimension; i++){
    RealT temp = p1->coordinates[i] - p2->coordinates[i];
#ifdef USE_L1_DISTANCE
    result += ABS(temp);
#else
    result += SQR(temp);
#endif
    if (result > threshold){
      // TIMEV_END(timeDistanceComputation);
      return 0;
    }
  }
  TIMEV_END(timeDistanceComputation);

  //return result <= threshold;
  return 1;
}

// // Returns TRUE iff |p1-p2|_2^2 <= threshold
// inline BooleanT isDistanceSqrLeq(IntT dimension, PPointT p1, PPointT p2, RealT threshold){
//   RealT result = 0;
//   nOfDistComps++;

//   //TIMEV_START(timeDistanceComputation);
//   for (IntT i = 0; i < dimension; i++){
//     result += p1->coordinates[i] * p2->coordinates[i];
//   }
//   //TIMEV_END(timeDistanceComputation);

//   return p1->sqrLength + p2->sqrLength - 2 * result <= threshold;
// }

// Returns the list of near neighbors of the point <point> (with a
// certain success probability). Near neighbor is defined as being a
// point within distance <parameterR>. Each near neighbor from the
// data set is returned is returned with a certain probability,
// dependent on <parameterK>, <parameterL>, and <parameterT>. The
// returned points are kept in the array <result>. If result is not
// allocated, it will be allocated to at least some minimum size
// (RESULT_INIT_SIZE). If number of returned points is bigger than the
// size of <result>, then the <result> is resized (to up to twice the
// number of returned points). The return value is the number of
// points found.

Int32T FgetNearNeighborsFromPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT query, PPointT *(&result), Int32T &resultSize, int &num, int subdim){
  ASSERT(nnStruct != NULL);
  ASSERT(query != NULL);
  ASSERT(nnStruct->reducedPoint != NULL);
  ASSERT(!nnStruct->useUfunctions || nnStruct->pointULSHVectors != NULL);

  PPointT point = query;

  if (result == NULL){
    resultSize = RESULT_INIT_SIZE;
    FAILIF(NULL == (result = (PPointT*)MALLOC(resultSize * sizeof(PPointT))));
  }

  //*******************
  // ACHash
  //*******************
  if(is_power_of_two(nnStruct->dimension)){
    double firHT[nnStruct->dimension];
    double *tmp = query->coordinates; 
    first_hadamard_transform(tmp, nnStruct->dimension, firHT);
    FpreparePointAdding(nnStruct, nnStruct->hashedBuckets[0], firHT, subdim);
  }else{
    int dimension = pow(2, ceil(log2(nnStruct->dimension)));
    double firHT[dimension];
    double tmp[dimension] = {0}; 
    for (IntT d = 0; d < nnStruct->dimension; ++d){tmp[d] = query->coordinates[d];}
    first_hadamard_transform(tmp, dimension, firHT);
    FpreparePointAdding(nnStruct, nnStruct->hashedBuckets[0], firHT, subdim);
  }
  Uns32T precomputedHashesOfULSHs[nnStruct->nHFTuples][N_PRECOMPUTED_HASHES_NEEDED];
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < N_PRECOMPUTED_HASHES_NEEDED; j++){
      precomputedHashesOfULSHs[i][j] = nnStruct->precomputedHashesOfULSHs[i][j];
    }
  }

  TIMEV_START(timeTotalBuckets);

  BooleanT oldTimingOn = timingOn;
  if (noExpensiveTiming) {
    timingOn = FALSE;
  }
  
  
  IntT firstUComp = 0;
  IntT secondUComp = 1;

  Int32T nNeighbors = 0;
  Int32T nMarkedPoints = 0;

  for(IntT i = 0; i < nnStruct->parameterL; i++){ 
    TIMEV_START(timeGetBucket);
    GeneralizedPGBucket gbucket;
    if (!nnStruct->useUfunctions) {
      // Use usual <g> functions (truly independent; <g>s are precisly
      // <u>s).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 1, precomputedHashesOfULSHs[i], NULL);
    } else {
      // Use <u> functions (<g>s are pairs of <u> functions).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 2, precomputedHashesOfULSHs[firstUComp], precomputedHashesOfULSHs[secondUComp]);
      secondUComp++;
      if (secondUComp == nnStruct->nHFTuples) {
	      firstUComp++;
	      secondUComp = firstUComp + 1;
      }
    }
    TIMEV_END(timeGetBucket);

    PGBucketT bucket;

    

    TIMEV_START(timeCycleBucket);
    switch (nnStruct->hashedBuckets[i]->typeHT){
      case HT_LINKED_LIST:
        bucket = gbucket.llGBucket;
        if (bucket != NULL){
	        PBucketEntryT bucketEntry = &(bucket->firstEntry);
	        while (bucketEntry != NULL){
	          Int32T candidatePIndex = bucketEntry->pointIndex;
	          PPointT candidatePoint = nnStruct->points[candidatePIndex];
            // printf("dataindex is %d\n", candidatePIndex);
            
	          if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	            if (nnStruct->markedPoints[candidatePIndex] == FALSE) {
	              if (nNeighbors >= resultSize){
		              resultSize = 2 * resultSize;
		              result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	              } 
	              result[nNeighbors] = candidatePoint;
	              nNeighbors++;
	              nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	              nnStruct->markedPoints[candidatePIndex] = TRUE; 
	              nMarkedPoints++;
	            }
	          }else{

	          }
	          bucketEntry = bucketEntry->nextEntry;
	        }
        }
        break;
    case HT_STATISTICS:
      ASSERT(FALSE); 
      break;
    case HT_HYBRID_CHAINS:
      if (gbucket.hybridGBucket != NULL){
	      PHybridChainEntryT hybridPoint = gbucket.hybridGBucket;
	      Uns32T offset = 0;
	      if (hybridPoint->point.bucketLength == 0){
	        offset = 0;
	        for(IntT j = 0; j < N_FIELDS_PER_INDEX_OF_OVERFLOW; j++){
	          offset += ((Uns32T)((hybridPoint + 1 + j)->point.bucketLength) << (j * N_BITS_FOR_BUCKET_LENGTH));
	        }
	      }
	      Uns32T index = 0;
	      BooleanT done = FALSE;
	      while(!done){
	        if (index == MAX_NONOVERFLOW_POINTS_PER_BUCKET){
	          index = index + offset;
	        }
	        Int32T candidatePIndex = (hybridPoint + index)->point.pointIndex;

	        CR_ASSERT(candidatePIndex >= 0 && candidatePIndex < nnStruct->nPoints);
	        done = (hybridPoint + index)->point.isLastPoint == 1 ? TRUE : FALSE;
	        index++;

          // printf("candidata index is %d\n", candidatePIndex);
          
	        if (nnStruct->markedPoints[candidatePIndex] == FALSE){
	          nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	          nnStruct->markedPoints[candidatePIndex] = TRUE; 
	          nMarkedPoints++;

            // printf("candidata index is %d\n", candidatePIndex);

	          PPointT candidatePoint = nnStruct->points[candidatePIndex];
            
	          if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	            if (nNeighbors >= resultSize){
		            resultSize = 2 * resultSize;
		            result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	            }
	            result[nNeighbors] = candidatePoint;
	            nNeighbors++;
	          }
	        }else{

	        }
	      }
      }
      break;
      default:
      ASSERT(FALSE);
    }
    TIMEV_END(timeCycleBucket);
    
    
  }
  printf("%d ", nMarkedPoints);
  num = nMarkedPoints;

  timingOn = oldTimingOn;
  TIMEV_END(timeTotalBuckets);

  // we need to clear the array nnStruct->nearPoints for the next query.
  for(Int32T i = 0; i < nMarkedPoints; i++){
    ASSERT(nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] == TRUE);
    nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] = FALSE;
  }
  DPRINTF("nMarkedPoints: %d\n", nMarkedPoints);

  return nNeighbors;
}

Int32T R2getNearNeighborsFromPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT query, PPointT *(&result), Int32T &resultSize, int &num, int subdim){
  ASSERT(nnStruct != NULL);
  ASSERT(query != NULL);
  ASSERT(nnStruct->reducedPoint != NULL);
  ASSERT(!nnStruct->useUfunctions || nnStruct->pointULSHVectors != NULL);

  PPointT point = query;

  if (result == NULL){
    resultSize = RESULT_INIT_SIZE;
    FAILIF(NULL == (result = (PPointT*)MALLOC(resultSize * sizeof(PPointT))));
  }
  
  RpreparePointAdding(nnStruct, nnStruct->hashedBuckets[0], point, subdim);

  Uns32T precomputedHashesOfULSHs[nnStruct->nHFTuples][N_PRECOMPUTED_HASHES_NEEDED];
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < N_PRECOMPUTED_HASHES_NEEDED; j++){
      precomputedHashesOfULSHs[i][j] = nnStruct->precomputedHashesOfULSHs[i][j];
    }
  }




  TIMEV_START(timeTotalBuckets);

  BooleanT oldTimingOn = timingOn;
  if (noExpensiveTiming) {
    timingOn = FALSE;
  }
  
  
  IntT firstUComp = 0;
  IntT secondUComp = 1;

  Int32T nNeighbors = 0;
  Int32T nMarkedPoints = 0;

  for(IntT i = 0; i < nnStruct->parameterL; i++){ 
    TIMEV_START(timeGetBucket);
    GeneralizedPGBucket gbucket;
    if (!nnStruct->useUfunctions) {
      // Use usual <g> functions (truly independent; <g>s are precisly
      // <u>s).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 1, precomputedHashesOfULSHs[i], NULL);
    } else {
      // Use <u> functions (<g>s are pairs of <u> functions).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 2, precomputedHashesOfULSHs[firstUComp], precomputedHashesOfULSHs[secondUComp]);
      secondUComp++;
      if (secondUComp == nnStruct->nHFTuples) {
	      firstUComp++;
	      secondUComp = firstUComp + 1;
      }
    }
    TIMEV_END(timeGetBucket);

    PGBucketT bucket;

    

    TIMEV_START(timeCycleBucket);
    switch (nnStruct->hashedBuckets[i]->typeHT){
      case HT_LINKED_LIST:
        bucket = gbucket.llGBucket;
        if (bucket != NULL){
	        PBucketEntryT bucketEntry = &(bucket->firstEntry);
	        while (bucketEntry != NULL){
	          Int32T candidatePIndex = bucketEntry->pointIndex;
	          PPointT candidatePoint = nnStruct->points[candidatePIndex];
            // printf("dataindex is %d\n", candidatePIndex);
            
	          if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	            if (nnStruct->markedPoints[candidatePIndex] == FALSE) {
	              if (nNeighbors >= resultSize){
		              resultSize = 2 * resultSize;
		              result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	              } 
	              result[nNeighbors] = candidatePoint;
	              nNeighbors++;
	              nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	              nnStruct->markedPoints[candidatePIndex] = TRUE; 
	              nMarkedPoints++;
	            }
	          }else{

	          }
	          bucketEntry = bucketEntry->nextEntry;
	        }
        }
        break;
    case HT_STATISTICS:
      ASSERT(FALSE); 
      break;
    case HT_HYBRID_CHAINS:
      if (gbucket.hybridGBucket != NULL){
	      PHybridChainEntryT hybridPoint = gbucket.hybridGBucket;
	      Uns32T offset = 0;
	      if (hybridPoint->point.bucketLength == 0){
	        offset = 0;
	        for(IntT j = 0; j < N_FIELDS_PER_INDEX_OF_OVERFLOW; j++){
	          offset += ((Uns32T)((hybridPoint + 1 + j)->point.bucketLength) << (j * N_BITS_FOR_BUCKET_LENGTH));
	        }
	      }
	      Uns32T index = 0;
	      BooleanT done = FALSE;
	      while(!done){
	        if (index == MAX_NONOVERFLOW_POINTS_PER_BUCKET){
	          index = index + offset;
	        }
	        Int32T candidatePIndex = (hybridPoint + index)->point.pointIndex;

	        CR_ASSERT(candidatePIndex >= 0 && candidatePIndex < nnStruct->nPoints);
	        done = (hybridPoint + index)->point.isLastPoint == 1 ? TRUE : FALSE;
	        index++;

          // printf("candidata index is %d\n", candidatePIndex);
          
	        if (nnStruct->markedPoints[candidatePIndex] == FALSE){
	          nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	          nnStruct->markedPoints[candidatePIndex] = TRUE; 
	          nMarkedPoints++;

            // printf("candidata index is %d\n", candidatePIndex);

	          PPointT candidatePoint = nnStruct->points[candidatePIndex];
            
	          if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	            if (nNeighbors >= resultSize){
		            resultSize = 2 * resultSize;
		            result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	            }
	            result[nNeighbors] = candidatePoint;
	            nNeighbors++;
	          }
	        }else{

	        }
	      }
      }
      break;
      default:
      ASSERT(FALSE);
    }
    TIMEV_END(timeCycleBucket);
    
    
  }
  printf("%d ", nMarkedPoints);
  num = nMarkedPoints; //printf("%d\n", nMarkedPoints);
  //printf("%d\n",Pratio[num]);

  timingOn = oldTimingOn;
  TIMEV_END(timeTotalBuckets);

  // we need to clear the array nnStruct->nearPoints for the next query.
  for(Int32T i = 0; i < nMarkedPoints; i++){
    ASSERT(nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] == TRUE);
    nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] = FALSE;
  }
  DPRINTF("nMarkedPoints: %d\n", nMarkedPoints);

  return nNeighbors;
}

Int32T getNearNeighborsFromPRNearNeighborStruct(PRNearNeighborStructT nnStruct, PPointT query, PPointT *(&result), Int32T &resultSize, int &num){
  ASSERT(nnStruct != NULL);
  ASSERT(query != NULL);
  ASSERT(nnStruct->reducedPoint != NULL);
  ASSERT(!nnStruct->useUfunctions || nnStruct->pointULSHVectors != NULL);

  PPointT point = query;

  if (result == NULL){
    resultSize = RESULT_INIT_SIZE;
    FAILIF(NULL == (result = (PPointT*)MALLOC(resultSize * sizeof(PPointT))));
  }
  
  preparePointAdding(nnStruct, nnStruct->hashedBuckets[0], point);

  Uns32T precomputedHashesOfULSHs[nnStruct->nHFTuples][N_PRECOMPUTED_HASHES_NEEDED];
  for(IntT i = 0; i < nnStruct->nHFTuples; i++){
    for(IntT j = 0; j < N_PRECOMPUTED_HASHES_NEEDED; j++){
      precomputedHashesOfULSHs[i][j] = nnStruct->precomputedHashesOfULSHs[i][j];
    }
  }


  TIMEV_START(timeTotalBuckets);

  BooleanT oldTimingOn = timingOn;
  if (noExpensiveTiming) {
    timingOn = FALSE;
  }
  
  // Initialize the counters for defining the pair of <u> functions used for <g> functions.
  IntT firstUComp = 0;
  IntT secondUComp = 1;

  Int32T nNeighbors = 0;// the number of near neighbors found so far.
  Int32T nMarkedPoints = 0;// the number of marked points
  

  for(IntT i = 0; i < nnStruct->parameterL; i++){ 
    TIMEV_START(timeGetBucket);
    GeneralizedPGBucket gbucket;
    if (!nnStruct->useUfunctions) {
      // Use usual <g> functions (truly independent; <g>s are precisly
      // <u>s).
      gbucket = getGBucket(nnStruct->hashedBuckets[i], 1, precomputedHashesOfULSHs[i], NULL);
    } else {
        // Use <u> functions (<g>s are pairs of <u> functions).
        gbucket = getGBucket(nnStruct->hashedBuckets[i], 2, precomputedHashesOfULSHs[firstUComp], precomputedHashesOfULSHs[secondUComp]);

        // compute what is the next pair of <u> functions.
        secondUComp++;
        if (secondUComp == nnStruct->nHFTuples) {
	        firstUComp++;
	        secondUComp = firstUComp + 1;
        }
      }



      TIMEV_END(timeGetBucket);

      PGBucketT bucket;

      TIMEV_START(timeCycleBucket);
      switch (nnStruct->hashedBuckets[i]->typeHT){
      case HT_LINKED_LIST:
      bucket = gbucket.llGBucket;
      if (bucket != NULL){
	      // circle through the bucket and add to <result> the points that are near.
	      PBucketEntryT bucketEntry = &(bucket->firstEntry);
	      //TIMEV_START(timeCycleProc);
	      while (bucketEntry != NULL){
	        //TIMEV_END(timeCycleProc);
	        //ASSERT(bucketEntry->point != NULL);
	        //TIMEV_START(timeDistanceComputation);
	        Int32T candidatePIndex = bucketEntry->pointIndex;
	        PPointT candidatePoint = nnStruct->points[candidatePIndex];
	        if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	          //TIMEV_END(timeDistanceComputation);
	          if (nnStruct->markedPoints[candidatePIndex] == FALSE) {
	            //TIMEV_START(timeResultStoring);
	            // a new R-NN point was found (not yet in <result>).
	            if (nNeighbors >= resultSize){
		            // run out of space => resize the <result> array.
		            resultSize = 2 * resultSize;
		            result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	            }
	            result[nNeighbors] = candidatePoint;
	            nNeighbors++;
	            nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	            nnStruct->markedPoints[candidatePIndex] = TRUE; // do not include more points with the same index
	            nMarkedPoints++;
	            //TIMEV_END(timeResultStoring);
	          }
	        }else{
	          //TIMEV_END(timeDistanceComputation);
	        }
	        //TIMEV_START(timeCycleProc);
	        bucketEntry = bucketEntry->nextEntry;
	      }
	      //TIMEV_END(timeCycleProc);
      }
      break;
    case HT_STATISTICS:
      ASSERT(FALSE); // HT_STATISTICS not supported anymore

//       if (gbucket.linkGBucket != NULL && gbucket.linkGBucket->indexStart != INDEX_START_EMPTY){
// 	Int32T position;
// 	PointsListEntryT *pointsList = nnStruct->hashedBuckets[i]->bucketPoints.pointsList;
// 	position = gbucket.linkGBucket->indexStart;
// 	// circle through the bucket and add to <result> the points that are near.
// 	while (position != INDEX_START_EMPTY){
// 	  PPointT candidatePoint = pointsList[position].point;
// 	  if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
// 	    if (nnStruct->nearPoints[candidatePoint->index] == FALSE) {
// 	      // a new R-NN point was found (not yet in <result>).
// 	      if (nNeighbors >= resultSize){
// 		// run out of space => resize the <result> array.
// 		resultSize = 2 * resultSize;
// 		result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
// 	      }
// 	      result[nNeighbors] = candidatePoint;
// 	      nNeighbors++;
// 	      nnStruct->nearPoints[candidatePoint->index] = TRUE; // do not include more points with the same index
// 	    }
// 	  }
// 	  // Int32T oldP = position;
// 	  position = pointsList[position].nextPoint;
// 	  // ASSERT(position == INDEX_START_EMPTY || position == oldP + 1);
// 	}
//       }
      break;
    case HT_HYBRID_CHAINS:
      if (gbucket.hybridGBucket != NULL){
	      PHybridChainEntryT hybridPoint = gbucket.hybridGBucket;
	      Uns32T offset = 0;
	      if (hybridPoint->point.bucketLength == 0){
	        // there are overflow points in this bucket.
	        offset = 0;
	        for(IntT j = 0; j < N_FIELDS_PER_INDEX_OF_OVERFLOW; j++){
	          offset += ((Uns32T)((hybridPoint + 1 + j)->point.bucketLength) << (j * N_BITS_FOR_BUCKET_LENGTH));
	        }
	      }
	      Uns32T index = 0;
	      BooleanT done = FALSE;
	      while(!done){
	        if (index == MAX_NONOVERFLOW_POINTS_PER_BUCKET){
	          //CR_ASSERT(hybridPoint->point.bucketLength == 0);
	          index = index + offset;
	        }
	        Int32T candidatePIndex = (hybridPoint + index)->point.pointIndex;
	        CR_ASSERT(candidatePIndex >= 0 && candidatePIndex < nnStruct->nPoints);
	        done = (hybridPoint + index)->point.isLastPoint == 1 ? TRUE : FALSE;
	        index++;

          // printf("candidata index is %d\n", candidatePIndex);
    
	        if (nnStruct->markedPoints[candidatePIndex] == FALSE){
	          // mark the point first.
	          nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	          nnStruct->markedPoints[candidatePIndex] = TRUE; // do not include more points with the same index
	          nMarkedPoints++;

	          PPointT candidatePoint = nnStruct->points[candidatePIndex];
	          if (isDistanceSqrLeq(nnStruct->dimension, point, candidatePoint, nnStruct->parameterR2) && nnStruct->reportingResult){
	            //if (nnStruct->markedPoints[candidatePIndex] == FALSE) {
	            // a new R-NN point was found (not yet in <result>).
	            //TIMEV_START(timeResultStoring);
	            if (nNeighbors >= resultSize){
		            // run out of space => resize the <result> array.
		            resultSize = 2 * resultSize;
		            result = (PPointT*)REALLOC(result, resultSize * sizeof(PPointT));
	            }
	            result[nNeighbors] = candidatePoint;
	            nNeighbors++;
              // printf("nNeighbors is %d\n", nNeighbors);
	            //TIMEV_END(timeResultStoring);
	            //nnStruct->markedPointsIndeces[nMarkedPoints] = candidatePIndex;
	            //nnStruct->markedPoints[candidatePIndex] = TRUE; // do not include more points with the same index
	            //nMarkedPoints++;
	            //}
	          }
	        }else{
	        // the point was already marked (& examined)
	      }
	    }
    }
      break;
    default:
      ASSERT(FALSE);
    }
    TIMEV_END(timeCycleBucket);
    
  }
  printf("%d ", nMarkedPoints);
  num = nMarkedPoints;

  timingOn = oldTimingOn;
  TIMEV_END(timeTotalBuckets);

  // we need to clear the array nnStruct->nearPoints for the next query.
  for(Int32T i = 0; i < nMarkedPoints; i++){
    ASSERT(nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] == TRUE);
    nnStruct->markedPoints[nnStruct->markedPointsIndeces[i]] = FALSE;
  }
  DPRINTF("nMarkedPoints: %d\n", nMarkedPoints);

  return nNeighbors;
}
