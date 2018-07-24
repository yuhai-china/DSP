/*
 * DwSolverMpi.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: kibaekkim
 */

//#define DSP_DEBUG
#include "AlpsKnowledgeBrokerSerial.h"
#include <Model/TssModel.h>
#include <DantzigWolfe/DwSolverMpi.h>
#include <DantzigWolfe/DwMaster.h>
#include <DantzigWolfe/DwBundleDual.h>
#include <DantzigWolfe/DwBundleDualSmip.h>
#include <DantzigWolfe/DwWorkerMpi.h>
#ifdef HAS_PIPS
#include <DantzigWolfe/DwBundleDualPips.h>
#include <DantzigWolfe/DwWorkerPips.h>
#endif

DwSolverMpi::DwSolverMpi(
		DecModel*   model,   /**< model pointer */
		DspParams*  par,     /**< parameters */
		DspMessage* message, /**< message pointer */
		MPI_Comm    comm     /**< MPI communicator */):
DwSolverSerial(model, par, message), comm_(comm) {
	MPI_Comm_size(comm_, &comm_size_);
	MPI_Comm_rank(comm_, &comm_rank_);
}

DwSolverMpi::~DwSolverMpi() {
	comm_ = MPI_COMM_NULL;
}

DSP_RTN_CODE DwSolverMpi::init() {
	BGN_TRY_CATCH

	/** create worker */
	if (model_->isStochastic()) {
#ifdef HAS_PIPS
		if (par_->getBoolParam("DW/MASTER/PIPS"))
			worker_ = new DwWorkerPips(model_, par_, message_, comm_);
		else
#endif
			worker_ = new DwWorkerMpi(model_, par_, message_, comm_);
	}
	else
		worker_ = new DwWorkerMpi(model_, par_, message_, comm_);

	if (comm_rank_ == 0) {
		/** create master */
		if (model_->isStochastic()) {
#ifdef HAS_PIPS
			if (par_->getBoolParam("DW/MASTER/PIPS"))
				master_ = new DwBundleDualPips(worker_);
			else
#endif
				master_ = new DwBundleDualSmip(worker_);
		} else
			master_ = new DwBundleDual(worker_);

		/** initialize master */
		DSP_RTN_CHECK_THROW(master_->init());

		/** create an Alps model */
		alps_ = new DwModel(master_);

		/** parameter setting */
		DspParams* par = alps_->getSolver()->getParPtr();
		alps_->AlpsPar()->setEntry(AlpsParams::searchStrategy, par->getIntParam("ALPS/SEARCH_STRATEGY"));
		alps_->AlpsPar()->setEntry(AlpsParams::nodeLogInterval, par->getIntParam("ALPS/NODE_LOG_INTERVAL"));
		alps_->AlpsPar()->setEntry(AlpsParams::nodeLimit, par->getIntParam("ALPS/NODE_LIM"));
		alps_->AlpsPar()->setEntry(AlpsParams::timeLimit, par->getDblParam("ALPS/TIME_LIM"));
		alps_->AlpsPar()->setEntry(AlpsParams::clockType, AlpsClockTypeWallClock);
	}
	END_TRY_CATCH_RTN(;,DSP_RTN_ERR)
	return DSP_RTN_OK;
}

DSP_RTN_CODE DwSolverMpi::solve() {
	BGN_TRY_CATCH
	if (comm_rank_ == 0) {

		/** solve */
		AlpsKnowledgeBrokerSerial alpsBroker(0, NULL, *alps_);
		alpsBroker.search(alps_);
		// alpsBroker.printBestSolution();

		DspNodeSolution* solution = NULL;
		if (alpsBroker.hasKnowledge(AlpsKnowledgeTypeSolution)) {
			solution = dynamic_cast<DspNodeSolution*>(alpsBroker.getBestKnowledge(AlpsKnowledgeTypeSolution).first);
			bestprimsol_ = solution->solution_;
			if (model_->isStochastic()) {
				TssModel* tss = dynamic_cast<TssModel*>(model_);
				if (bestprimsol_.size() > 0)
					bestprimsol_.erase(bestprimsol_.begin(), bestprimsol_.begin() + tss->getNumCols(0) * (tss->getNumScenarios() - 1));
			}
		}
		bestprimobj_ = alpsBroker.getBestQuality();
		bestdualobj_ = alps_->getBestDualObjective();

		/** send signal */
		int sig = DwWorkerMpi::sig_terminate;
		MPI_Bcast(&sig, 1, MPI_INT, 0, comm_);
		DSPdebugMessage("Rank 0 sent signal %d.\n", sig);
	} else {
		DSP_RTN_CHECK_THROW(dynamic_cast<DwWorkerMpi*>(worker_)->receiver());
	}
	END_TRY_CATCH_RTN(;,DSP_RTN_ERR)
	return DSP_RTN_OK;
}

DSP_RTN_CODE DwSolverMpi::finalize() {
	BGN_TRY_CATCH
	if (comm_rank_ == 0)
		DSP_RTN_CHECK_THROW(master_->finalize());
	END_TRY_CATCH_RTN(;,DSP_RTN_ERR)
	return DSP_RTN_OK;
}
