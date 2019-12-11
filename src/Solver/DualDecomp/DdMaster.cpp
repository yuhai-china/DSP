/*
 * DdMaster.cpp
 *
 *  Created on: Feb 9, 2016
 *      Author: kibaekkim
 */

#include "CoinHelperFunctions.hpp"
#include "Solver/DualDecomp/DdMaster.h"

DdMaster::DdMaster(DecModel* model, DspParams* par, DspMessage* message) :
		DecSolver(model, par, message),
lambda_(NULL),
subsolution_(NULL) {
	/**< nothing to do */
}

DdMaster::DdMaster(const DdMaster& rhs) :
DecSolver(rhs),
lambda_(rhs.lambda_) {
	subsolution_ = new double * [model_->getNumSubproblems()];
	for (int s = 0; s < model_->getNumSubproblems(); ++s) {
		subsolution_[s] = new double [model_->getNumSubproblemCouplingCols(s)];
		CoinCopyN(rhs.subsolution_[s], model_->getNumSubproblemCouplingCols(s), subsolution_[s]);
	}
}

DdMaster::~DdMaster() {
	lambda_ = NULL;
	FREE_2D_ARRAY_PTR(model_->getNumSubproblems(),subsolution_);
}

DSP_RTN_CODE DdMaster::init() {
	BGN_TRY_CATCH

	/** status */
	status_ = DSP_STAT_MW_CONTINUE;

	/** time stamp */
	ticToc();

	/** allocate memory */
	subsolution_ = new double * [model_->getNumSubproblems()];
	for (int s = 0; s < model_->getNumSubproblems(); ++s)
		subsolution_[s] = new double [model_->getNumSubproblemCouplingCols(s)];

	/** initialize */
	primsol_.resize(model_->getNumCouplingCols());
	bestprimsol_.resize(model_->getFullModelNumCols());
	bestdualsol_.resize(model_->getNumCouplingRows());
	subprimobj_.resize(model_->getNumSubproblems());
	subdualobj_.resize(model_->getNumSubproblems());

	END_TRY_CATCH_RTN(;,DSP_RTN_ERR)

	return DSP_RTN_OK;
}


/** set init solution */
DSP_RTN_CODE DdMaster::setInitSolution(const double * sol) {
	BGN_TRY_CATCH

	if (primsol_.size() >= si_->getNumCols())
		CoinCopyN(sol, si_->getNumCols(), &primsol_[0]);

	END_TRY_CATCH_RTN(;,DSP_RTN_ERR)

	return DSP_RTN_OK;
}
