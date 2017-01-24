#include "bitcalc.h"
#include "wrapperMPI.h"
#include "mltplyMPI.h"
#include "mltply.h"
#ifdef MPI
#include "mfmemory.h"
#endif

///
/// Calculation of pair excited state for Hubbard Grand canonical system
/// \param X define list to get and put information of calculation
/// \param tmp_v0 [out] Result v0 = H v1
/// \param tmp_v1 [in] v0 = H v1
/// \returns TRUE: Normally finished
/// \returns FALSE: Abnormally finished
/// \authour Kazuyoshi Yoshimi
/// \version 1.2
int GetPairExcitedStateHubbardGC(
        struct BindStruct *X,
        double complex *tmp_v0, /**< [out] Result v0 = H v1*/
        double complex *tmp_v1 /**< [in] v0 = H v1*/

){

    long unsigned int i,j;
    long unsigned int isite1;
    long unsigned int org_isite1,org_isite2,org_sigma1,org_sigma2;

    double complex tmp_trans=0;
    long int i_max;
    long int ibit;
    long unsigned int is;
    i_max = X->Check.idim_maxOrg;
    for(i=0;i<X->Def.NPairExcitationOperator;i++) {
        org_isite1 = X->Def.PairExcitationOperator[i][0] + 1;
        org_isite2 = X->Def.PairExcitationOperator[i][2] + 1;
        org_sigma1 = X->Def.PairExcitationOperator[i][1];
        org_sigma2 = X->Def.PairExcitationOperator[i][3];
        tmp_trans = X->Def.ParaPairExcitationOperator[i];

        if (org_isite1 > X->Def.Nsite &&
            org_isite2 > X->Def.Nsite) {
            if (org_isite1 == org_isite2 && org_sigma1 == org_sigma2) {
                if (X->Def.PairExcitationOperator[i][4] == 0) {
                    if (org_sigma1 == 0) {
                        is = X->Def.Tpow[2 * org_isite1 - 2];
                    }
                    else {
                        is = X->Def.Tpow[2 * org_isite1 - 1];
                    }
                    ibit = (unsigned long int) myrank & is;
                    if (ibit == is) {
#pragma omp parallel for default(none) shared(tmp_v0, tmp_v1)	\
  firstprivate(i_max, tmp_trans) private(j)
                        for (j = 1; j <= i_max; j++) tmp_v0[j] += tmp_trans * tmp_v1[j];
                    }
                }
                else {//X->Def.PairExcitationOperator[i][4]==1
                    if (org_sigma1 == 0) {
                        is = X->Def.Tpow[2 * org_isite1 - 2];
                    }
                    else {
                        is = X->Def.Tpow[2 * org_isite1 - 1];
                    }
                    ibit = (unsigned long int) myrank & is;
                    if (ibit != is) {
                        //minus sign comes from negative tmp_trans due to readdef
#pragma omp parallel for default(none) shared(tmp_v0, tmp_v1)	\
  firstprivate(i_max, tmp_trans) private(j)
                        for (j = 1; j <= i_max; j++) tmp_v0[j] += -tmp_trans * tmp_v1[j];
                    }
                }
            }
            else {
                X_GC_child_general_hopp_MPIdouble(org_isite1 - 1, org_sigma1, org_isite2 - 1, org_sigma2, -tmp_trans, X,
                                                  tmp_v0, tmp_v1);
            }
        }
        else if (org_isite2 > X->Def.Nsite || org_isite1 > X->Def.Nsite) {
            if (org_isite1 < org_isite2) {
                X_GC_child_general_hopp_MPIsingle(org_isite1 - 1, org_sigma1, org_isite2 - 1, org_sigma2, -tmp_trans, X,
                                                  tmp_v0, tmp_v1);
            }
            else {
                X_GC_child_general_hopp_MPIsingle(org_isite2 - 1, org_sigma2, org_isite1 - 1, org_sigma1,
                                                  -conj(tmp_trans), X, tmp_v0, tmp_v1);
            }
        }
        else {

            if (org_isite1 == org_isite2 && org_sigma1 == org_sigma2 && X->Def.PairExcitationOperator[i][4] == 1) {
                isite1=X->Def.Tpow[2 * org_isite1 - 2 + org_sigma1];
#pragma omp parallel for default(none) private(j) firstprivate(i_max,X,isite1, tmp_trans) shared(tmp_v0, tmp_v1)
                for(j=1;j<=i_max;j++){
                    GC_AisCis(j, tmp_v0, tmp_v1, X, isite1, -tmp_trans);
                }
            }
            else {
                if (child_general_hopp_GetInfo(X, org_isite1, org_isite2, org_sigma1, org_sigma2) != 0) {
                    return -1;
                }
                GC_child_general_hopp(tmp_v0, tmp_v1, X, tmp_trans);
            }
        }
    }
    return TRUE;
}

///
/// Calculation of pair excited state for Hubbard canonical system
/// \param X define list to get and put information of calculation
/// \param tmp_v0 [out] Result v0 = H v1
/// \param tmp_v1 [in] v0 = H v1
/// \returns TRUE: Normally finished
/// \returns FALSE: Abnormally finished
/// \authour Kazuyoshi Yoshimi
/// \version 1.2
int GetPairExcitedStateHubbard(
        struct BindStruct *X,
        double complex *tmp_v0, /**< [out] Result v0 = H v1*/
        double complex *tmp_v1 /**< [in] v0 = H v1*/
){
    long unsigned int i,j, idim_maxMPI;
    long unsigned int irght,ilft,ihfbit;
    long unsigned int isite1;
    long unsigned int org_isite1,org_isite2,org_sigma1,org_sigma2;
    long unsigned int tmp_off=0;
    long unsigned int off=0;

    double complex tmp_trans=0;
    long int i_max;
    int tmp_sgn, num1;
    long int ibit1, ibit;
    long unsigned int is1_up, is, Asum, Adiff;
    long unsigned int ibitsite1, ibitsite2;

    //  i_max = X->Check.idim_max;
    i_max = X->Check.idim_maxOrg;
    if(GetSplitBitByModel(X->Def.Nsite, X->Def.iCalcModel, &irght, &ilft, &ihfbit)!=0){
        return -1;
    }
    X->Large.i_max    = i_max;
    X->Large.irght    = irght;
    X->Large.ilft     = ilft;
    X->Large.ihfbit   = ihfbit;
    X->Large.mode=M_CALCSPEC;
//    X->Large.mode     = M_MLTPLY;

    double complex *tmp_v1bufOrg;
    //set size
#ifdef MPI
    idim_maxMPI = MaxMPI_li(X->Check.idim_maxOrg);
    c_malloc1(tmp_v1bufOrg, idim_maxMPI + 1);
#endif // MPI

    for(i=0;i<X->Def.NPairExcitationOperator;i++){
        org_isite1 = X->Def.PairExcitationOperator[i][0]+1;
        org_isite2 = X->Def.PairExcitationOperator[i][2]+1;
        org_sigma1 = X->Def.PairExcitationOperator[i][1];
        org_sigma2 = X->Def.PairExcitationOperator[i][3];
        tmp_trans = X->Def.ParaPairExcitationOperator[i];
        ibitsite1 = X->Def.OrgTpow[2*org_isite1-2+org_sigma1] ;
        ibitsite2 = X->Def.OrgTpow[2*org_isite2-2+org_sigma2] ;
        child_general_hopp_GetInfo(X, org_isite1, org_isite2, org_sigma1, org_sigma2);
        Asum = X->Large.isA_spin;
        Adiff = X->Large.A_spin;

        if(X->Def.iFlagListModified == TRUE // Not to adopt HubbrdNConserved
           && org_sigma1 != org_sigma2){
            if (org_isite1  > X->Def.Nsite &&
                org_isite2  > X->Def.Nsite)
            {
                X_child_CisAjt_MPIdouble(org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, -tmp_trans, X, tmp_v0, tmp_v1, tmp_v1bufOrg, list_1_org, list_1buf_org, list_2_1, list_2_2);
            }
            else if (org_isite2  > X->Def.Nsite
                     || org_isite1  > X->Def.Nsite)
            {
                if(org_isite1 < org_isite2) {
                    X_child_CisAjt_MPIsingle(org_isite1 - 1, org_sigma1, org_isite2 - 1, org_sigma2, -tmp_trans, X, tmp_v0,
                                             tmp_v1, tmp_v1bufOrg, list_1_org, list_1buf_org, list_2_1, list_2_2);
                } else{
                    X_child_CisAjt_MPIsingle(org_isite2 - 1, org_sigma2, org_isite1 - 1, org_sigma1, -conj(tmp_trans), X, tmp_v0,
                                             tmp_v1, tmp_v1bufOrg, list_1_org, list_1buf_org, list_2_1, list_2_2); }
            }
            else{
#pragma omp parallel for default(none) shared(tmp_v0, tmp_v1)	\
  firstprivate(i_max, tmp_trans, Asum, Adiff, ibitsite1, ibitsite2, X, list_1_org) \
  private(j, tmp_sgn, tmp_off)
                for (j = 1; j <= i_max; j++){
                    tmp_sgn=X_CisAjt(list_1_org[j], X, ibitsite1, ibitsite2, Asum, Adiff, &tmp_off);
                    tmp_v0[tmp_off] += tmp_trans * tmp_sgn*tmp_v1[j];
                }
            }
        }
        else{
            if (org_isite1  > X->Def.Nsite &&
                org_isite2  > X->Def.Nsite) {
                if(org_isite1==org_isite2 && org_sigma1==org_sigma2){//diagonal
                    is = X->Def.Tpow[2 * org_isite1 - 2 + org_sigma1];
                    ibit = (unsigned long int) myrank & is;
                    if( X->Def.PairExcitationOperator[i][4]==0) {
                        if (ibit == is) {
#pragma omp parallel for default(none) shared(tmp_v0, tmp_v1)	\
  firstprivate(i_max, tmp_trans) private(j)
                            for (j = 1; j <= i_max; j++) tmp_v0[j] += tmp_trans * tmp_v1[j];
                        }
                    }
                    else{
                        if (ibit != is) {
#pragma omp parallel for default(none) shared(tmp_v0, tmp_v1)	\
  firstprivate(i_max, tmp_trans) private(j)
                            for (j = 1; j <= i_max; j++) tmp_v0[j] += -tmp_trans * tmp_v1[j];
                        }
                    }
                }
                else{
                    X_child_general_hopp_MPIdouble(org_isite1-1, org_sigma1, org_isite2-1, org_sigma2, -tmp_trans, X, tmp_v0, tmp_v1);
                }
            }
            else if (org_isite2  > X->Def.Nsite || org_isite1  > X->Def.Nsite){
                if(org_isite1 < org_isite2){
                    X_child_general_hopp_MPIsingle(org_isite1-1, org_sigma1,org_isite2-1, org_sigma2, -tmp_trans, X, tmp_v0, tmp_v1);
                }
                else{
                    X_child_general_hopp_MPIsingle(org_isite2-1, org_sigma2, org_isite1-1, org_sigma1, -conj(tmp_trans), X, tmp_v0, tmp_v1);
                }
            }
            else{
                if(child_general_hopp_GetInfo( X,org_isite1,org_isite2,org_sigma1,org_sigma2)!=0){
                    return -1;
                }
                if(org_isite1==org_isite2 && org_sigma1==org_sigma2){
                    is = X->Def.Tpow[2 * org_isite1 - 2 + org_sigma1];
                    if( X->Def.PairExcitationOperator[i][4]==0) {
#pragma omp parallel for default(none) shared(list_1, tmp_v0, tmp_v1) firstprivate(i_max, is, tmp_trans) private(num1, ibit)
                        for (j = 1; j <= i_max; j++) {
                            ibit = list_1[j] & is;
                            num1 = ibit / is;
                            tmp_v0[j] += tmp_trans * num1 * tmp_v1[j];
                        }
                    }
                    else{
#pragma omp parallel for default(none) shared(list_1, tmp_v0, tmp_v1) firstprivate(i_max, is, tmp_trans) private(num1, ibit)
                        for (j = 1; j <= i_max; j++) {
                            ibit = list_1[j] & is;
                            num1 = (1-ibit / is);
                            tmp_v0[j] += -tmp_trans * num1 * tmp_v1[j];
                        }
                    }
                }
                else{
                    child_general_hopp(tmp_v0, tmp_v1,X,tmp_trans);
                }
            }
        }
    }
    return TRUE;
}