#include "NavierStokesEquations.h"
#include "../../../00_CommonFiles/Allocation_Operations/allocations.h"
#include "../../../00_CommonFiles/IO_Operations/io.h"


int Preprocess(int narg, char **arguments, ParametersType **Parameters_out, MatrixDataType **MatrixData_out, FemStructsType **FemStructs_out, FemFunctionsType **FemFunctions_out, FemOtherFunctionsType **FemOtherFunctions_out)
{
	int neq, neqpress, nnodes, nel, I, J;
	int tag = 1; // Testing input error
	int **lm, *lmaux, *eqpress;
	int size = NDOF*NNOEL;
	int size2 = size*size;
	double *F, *u;
	char FileName[300], label[300];
	FILE *InFile;
	NodeType *Node;
	ElementType *Element;
	ParametersType *Parameters;
	MatrixDataType *MatrixData;

	FemStructsType *FemStructs;
	FemFunctionsType *FemFunctions;
	FemOtherFunctionsType *FemOtherFunctions;
	
	/* **************************************************************************************************************************** */
	//												Testing initial parameters
	/* **************************************************************************************************************************** */
	if (narg!=2)
	{
		printf("Use ./NavierStokesEquations2D <Parameters file according README>\n");
		exit(1);
	}
	/* **************************************************************************************************************************** */


	/***************************************************************************/
	//			Reading parameters from problem setting file
	/***************************************************************************/
	Parameters   = mycalloc("Parameters of 'Preprocess'",1,sizeof(ParametersType));
	MatrixData   = mycalloc("MatrixData of 'Preprocess'",1,sizeof(MatrixDataType));
	FemStructs   = mycalloc("FemStructs of 'Preprocess'",1,sizeof(FemStructsType));
	FemFunctions = mycalloc("FemFunctions of 'Preprocess'",1,sizeof(FemFunctionsType));
	FemOtherFunctions = mycalloc("FemOtherFunctions of 'Preprocess'",1,sizeof(FemOtherFunctionsType));

	InFile = myfopen(arguments[1], "r");
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->Experiments, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->ProblemTitle, label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &Parameters->ReynoldsNumber, label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->SolverTolerance), label);
	tag = fscanf(InFile, "%d\t:%[^\n]", &(Parameters->SolverToleranceCase), label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->CorrectionTolerance), label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->TimeIntegrationTolerance), label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->CoefficientTolerance), label);
	tag = fscanf(InFile, "%d\t:%[^\n]", &(Parameters->LinearMaxIter), label);
	tag = fscanf(InFile, "%d\t:%[^\n]", &(Parameters->KrylovBasisVectorsQuantity), label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->Solver, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->Preconditioner, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->reordering, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->TimeIntegration, label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &Parameters->Alpha, label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &Parameters->DeltaT, label);
	tag = fscanf(InFile, "%lf\t:%[^\n]", &Parameters->FinalT, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->Dimensionless, label);
	tag = fscanf(InFile, "%d\t:%[^\n]", &Parameters->NumberCorrection, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->StopMulticorrection, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->MatrixVectorProductScheme, label);
	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->StabilizationForm, label);
//	tag = fscanf(InFile, "%s\t:%[^\n]", Parameters->ShockCapture, label);
//	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->invY[0]), label);
//	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->invY[1]), label);
//	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->invY[2]), label);
//	tag = fscanf(InFile, "%lf\t:%[^\n]", &(Parameters->invY[3]), label);
	tag = fscanf(InFile, "%d\t:%[^\n]", &(Parameters->nnodes), label);
	tag = fscanf(InFile, "%d\t:%[^\n]", &(Parameters->nel), label);
	fclose(InFile);
	
	/* **************************************************************************************************************************** */


	/* **************************************************************************************************************************** */
	//						Reading nodes
	/* **************************************************************************************************************************** */
	sprintf(FileName,"../02_mesh/%s_%d_%d.dat", Parameters->ProblemTitle, Parameters->nnodes, Parameters->nel);
	InFile = myfopen(FileName, "r");
	tag = fscanf(InFile, "%d", &nnodes);
	Node = (NodeType*) mycalloc("Node of 'Preprocess'", nnodes, sizeof(NodeType));
	for (I = 0; I < nnodes; I++)
	{
		tag = fscanf(InFile, "%lf%lf%d%d%d", &(Node[I].x), &(Node[I].y), &(Node[I].v1Type), &(Node[I].v2Type), &(Node[I].pType));
	}
	Fill_ID(&neq, &neqpress, Node, nnodes);
	
	/* **************************************************************************************************************************** */


	/* **************************************************************************************************************************** */
	//           				Reading connection mesh
	/* **************************************************************************************************************************** */
	tag = fscanf(InFile, "%d", &nel);
	Element = (ElementType*) mycalloc("Element of 'Preprocess'", nel, sizeof(ElementType));
	for (I = 0; I < nel; I++)
		tag = fscanf(InFile, "%d%d%d%d", &(Element[I].Vertex[0]), &(Element[I].Vertex[1]), &(Element[I].Vertex[2]), &(Element[I].Type));
	fclose(InFile);
	
	/* **************************************************************************************************************************** */


	/* **************************************************************************************************************************** */
	//			          Memory allocations and Store strategies 
	/* **************************************************************************************************************************** */

	// Some variable initializations
	

	Parameters->neq = neq;
	Parameters->nel = nel;
	Parameters->nnodes = nnodes; 
	Parameters->neqpress = neqpress;

	MatrixData = (MatrixDataType *) mycalloc("MatrixData of 'Preprocess'", 1, sizeof(MatrixDataType));
	F = (double*) mycalloc("F of 'Preprocess'", neq+1, sizeof(double));
	u = (double*) mycalloc("u of 'Preprocess'", neq+1, sizeof(double));
	lm = (int**) mycalloc("lm of 'Preprocess'", nel, sizeof(int*));
	lmaux = (int*) mycalloc("lmaux of 'Preprocess'", nel*size, sizeof(int));
	for (I = 0; I < nel; I++)
		lm[I] = &lmaux[I*size];
	
	
	// Set vector eqpress
	eqpress = (int*) mycalloc("eqpress of 'Preprocess'", neqpress, sizeof(int));
	J = 0;
	for (I = 0; I < nnodes; I++){
		if(Node[I].id[0] >= 0){ // a pressao é incógnita naquele nó
			eqpress[J] = Node[I].id[2];
			J++;
		}
	}


	//Configuring equation according to variables and boundary conditions
	Fill_LM(neq, nel, lm, Node, Element);
	FemStructs->lm = lm;
	FemStructs->lmaux = lmaux;
	
	if (strncmp(Parameters->MatrixVectorProductScheme,"EBE",3) == 0){
	
		double **M, *Maux;
		double *Diag, *invDiag;

		M = (double**) mycalloc("M of 'Preprocess'", nel, sizeof(double*));
		Maux = (double*) mycalloc("Maux of 'Preprocess'", nel*size2,sizeof(double));
		Diag = (double*) mycalloc("Diag of 'Preprocess'", neq+1, sizeof(double));
		invDiag = (double*) mycalloc("invDiag of 'Preprocess'", neq+1, sizeof(double));
		
		for (I = 0; I < nel; I++){
			M[I] = &Maux[I*size2];
		}

		MatrixData->A = M;
		MatrixData->Aaux = Maux;
		MatrixData->Diag = Diag;
		MatrixData->invDiag = invDiag;
	}
	else if (strcasecmp(Parameters->MatrixVectorProductScheme,"CSR") == 0){
		int *IA, *JA, *perm, *invperm, **CSR_by_Element;
		double *AA, *Diag, *invDiag;
			
		csr_Initialization(Parameters, Node, &JA, &IA, &perm, &invperm, &lm, &lmaux, &CSR_by_Element);
		
		AA = (double*) mycalloc("AA of 'Preprocess'", Parameters->nnzero+1, sizeof(double));
		Diag = (double*) mycalloc("Diag of 'Preprocess'", neq+1, sizeof(double));
		invDiag = (double*) mycalloc("invDiag of 'Preprocess'", neq+1, sizeof(double));
		
		printf("nnzero=%d\n",Parameters->nnzero);

		MatrixData->AA = AA;
		MatrixData->JA = JA;
		MatrixData->IA = IA;
		MatrixData->Diag = Diag;
		MatrixData->invDiag = invDiag;
		MatrixData->Scheme_by_Element = CSR_by_Element;
		MatrixData->Perm = perm;
		MatrixData->invPerm = invperm;

	}
	else{
		printf("Matrix vector product scheme not defined!\n\n");
		exit(1);
	}
	
	/* **************************************************************************************************************************** */

	FemStructs->Node = Node;
	FemStructs->Element = Element;
	FemStructs->F = F;
	FemStructs->u = u;
	FemStructs->eqpress = eqpress;
	
	*Parameters_out = Parameters;
	*MatrixData_out = MatrixData;
	
	*FemStructs_out = FemStructs;
	*FemFunctions_out = FemFunctions;
	*FemOtherFunctions_out = FemOtherFunctions;

	if (tag<0){
		printf ("Error in some parameter\n");
		exit(1);
	}
	
	return 0;
}


