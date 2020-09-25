/*
 * SCIPconshdlrBendersWorker.h
 *
 *  Created on: Feb 18, 2016
 *      Author: kibaekkim
 */

#ifndef SRC_SOLVERINTERFACE_SCIPCONSHDLRBENDERSWORKER_H_
#define SRC_SOLVERINTERFACE_SCIPCONSHDLRBENDERSWORKER_H_

#include "Utility/DspMpi.h"
#include "Solver/Benders/SCIPconshdlrBenders.h"

/** A class for implementing parallel Benders constraint handler */
class SCIPconshdlrBendersWorker: public SCIPconshdlrBenders {
public:

	/** default constructor */
	SCIPconshdlrBendersWorker(SCIP * scip, int sepapriority, bool is_integral_recourse, MPI_Comm comm);

	/** default destructor */
	virtual ~SCIPconshdlrBendersWorker();

	/** destructor of constraint handler to free user data (called when SCIP is exiting) */
	virtual SCIP_DECL_CONSFREE(scip_free);

	/** set model pointer */
	virtual void setDecModel(DecModel * model);

protected:

	/** generate Benders cuts */
	virtual void generateCuts(
			int size,      /**< [in] size of x */
			double * x,    /**< [in] master solution */
			int where,     /**< [in] where to be called */
			OsiCuts * cuts /**< [out] cuts generated */);

	/** generate Benders cuts */
	virtual void aggregateCuts(
			double ** cutvec, /**< [in] cut vector */
			double *  cutrhs, /**< [in] cut right-hand side */
			OsiCuts * cuts    /**< [out] cuts generated */);

	/** Has integer variables in the recouse? */
	virtual bool isIntegralRecourse() {
		return is_integral_recourse_;
	}

	/** evaluate recourse: 
	 * This function is called only when the recourse has integer variables.
	*/
	virtual SCIP_RETCODE evaluateRecourse(
			SCIP * scip,    /**< [in] scip pointer */
			SCIP_SOL * sol, /**< [in] solution to evaluate */
			double * values /**< [out] evaluated recourse values */);

private:

	MPI_Comm comm_;
	int comm_rank_;
	int comm_size_;

	/** cut communication */
	int * recvcounts_;
	int * displs_;
	int * cut_index_;  /**< subproblem index for which cut is generated */
	int * cut_status_; /**< cut generation status from MPI_Gatherv */

	bool is_integral_recourse_;
};

#endif /* SRC_SOLVERINTERFACE_SCIPCONSHDLRBENDERSWORKER_H_ */
