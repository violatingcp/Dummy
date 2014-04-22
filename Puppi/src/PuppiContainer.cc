#include "Dummy/Puppi/interface/PuppiContainer.h"
#include "fastjet/internal/base.hh"
#include "Math/ProbFunc.h"
#include "TMath.h"
#include <iostream>
#include <math.h>

PuppiContainer::PuppiContainer(const edm::ParameterSet &iConfig) {
  fApplyCHS        = iConfig.getUntrackedParameter<bool>("applyCHS"); 
  fUseDZ           = iConfig.getUntrackedParameter<bool>("useDZ");
  fNeutralMinPt    = iConfig.getUntrackedParameter<double>("MinNeutralPt");
  fPuppiWeightCut  = iConfig.getUntrackedParameter<double>("MinPuppiWeight");
  std::vector<edm::ParameterSet> lAlgos = iConfig.getParameter<std::vector<edm::ParameterSet> >("algos"); 
  fNAlgos = lAlgos.size();
  for(unsigned int i0 = 0; i0 < lAlgos.size(); i0++) { 
    PuppiAlgo pPuppiConfig(lAlgos[i0]);
    fPuppiAlgo.push_back(pPuppiConfig);
  }
}  
void PuppiContainer::initialize(const std::vector<RecoObj> &iRecoObjects) { 
    //Clear everything
    fRecoParticles.resize(0);
    fPFParticles  .resize(0);
    fChargedPV    .resize(0);
    fPupParticles .resize(0);
    fWeights      .resize(0);
    fVals.resize(0);
    //fChargedNoPV.resize(0);
    //Link to the RecoObjects
    fRecoParticles = iRecoObjects;
    for (unsigned int i = 0; i < fRecoParticles.size(); i++){
        fastjet::PseudoJet curPseudoJet;
        curPseudoJet.reset_PtYPhiM(fRecoParticles[i].pt,fRecoParticles[i].eta,fRecoParticles[i].phi,fRecoParticles[i].m);
        curPseudoJet.set_user_index(fRecoParticles[i].id);
        // fill vector of pseudojets for internal references
        fPFParticles.push_back(curPseudoJet);
	//Take Charged particles associated to PV
	if(fRecoParticles[i].id == 2) fChargedPV.push_back(curPseudoJet);
	//if((fRecoParticles[i].id == 0) && (inParticles[i].id == 2))  _genParticles.push_back( curPseudoJet);
	//if(fRecoParticles[i].id <= 2 && !(inParticles[i].pt < fNeutralMinE && fRecoParticles[i].id < 2)) _pfchsParticles.push_back(curPseudoJet); 
	//if(fRecoParticles[i].id == 3) _chargedNoPV.push_back(curPseudoJet);
    }
}
PuppiContainer::~PuppiContainer(){}
double PuppiContainer::goodVar(PseudoJet &iPart,std::vector<PseudoJet> &iParts, int iOpt,double iRCone) {
  double lPup = 0;
  lPup = var_within_R(iOpt,iParts,iPart,iRCone);
  return lPup;
}
//In fact takes the median no the average
void PuppiContainer::getRMSAvg(int iOpt,std::vector<fastjet::PseudoJet> &iConstits,std::vector<fastjet::PseudoJet> &iParticles,std::vector<fastjet::PseudoJet> &iChargedParticles) { 
  for(unsigned int i0 = 0; i0 < iConstits.size(); i0++ ) { 
    double pVal = -1;
    //Calculate the Puppi Algo to use
    int  pPupId   = getPuppiId(iConstits[i0].pt(),iConstits[i0].eta());
    if(fPuppiAlgo[pPupId].numAlgos() <= iOpt) pPupId = -1;
    if(pPupId == -1) {fVals.push_back(-1); continue;}
    //Get the Puppi Sub Algo (given iteration)
    int  pAlgo    = fPuppiAlgo[pPupId].algoId   (iOpt); 
    bool pCharged = fPuppiAlgo[pPupId].isCharged(iOpt);
    double pCone  = fPuppiAlgo[pPupId].coneSize (iOpt);
    //Compute the Puppi Metric 
    if(!pCharged) pVal = goodVar(iConstits[i0],iParticles       ,pAlgo,pCone);
    if( pCharged) pVal = goodVar(iConstits[i0],iChargedParticles,pAlgo,pCone);
    fVals.push_back(pVal);
    if(std::isnan(pVal) || std::isinf(pVal)) cerr << "====> Value is Nan " << pVal << " == " << iConstits[i0].pt() << " -- " << iConstits[i0].eta() << endl;
    //if(isnan(pVal) || isinf(pVal))  edm::LogError( "NotFound" )  << "====> Value is Nan " << pVal << " == " << iConstits[i0].pt() << " -- " << iConstits[i0].eta() << endl;
    if(std::isnan(pVal) || std::isinf(pVal)) continue;
    fPuppiAlgo[pPupId].add(iConstits[i0],pVal,iOpt);
  }
  for(int i0 = 0; i0 < fNAlgos; i0++) fPuppiAlgo[i0].computeMedRMS(iOpt);
}
int    PuppiContainer::getPuppiId(const float &iPt,const float &iEta) { 
  int lId = -1; 
  for(int i0 = 0; i0 < fNAlgos; i0++) { 
    if(iEta < fPuppiAlgo[i0].etaMin()) continue;
    if(iEta > fPuppiAlgo[i0].etaMax()) continue;
    if(iPt  < fPuppiAlgo[i0].ptMin())  continue;
    lId = i0; 
    break;
  }
  //if(lId == -1) std::cerr << "Error : Full fiducial range is not defined " << std::endl;
  return lId;
}
double PuppiContainer::getChi2FromdZ(double iDZ) { 
  //We need to obtain prob of PU + (1-Prob of LV)
  // Prob(LV) = Gaus(dZ,sigma) where sigma = 1.5mm  (its really more like 1mm)
  double lProbLV = ROOT::Math::normal_cdf_c(fabs(iDZ),0.2)*2.; //*2 is to do it double sided
  double lProbPU = 1-lProbLV;
  if(lProbPU <= 0) lProbPU = 1e-16;   //Quick Trick to through out infs
  if(lProbPU >= 0) lProbPU = 1-1e-16; //Ditto
  double lChi2PU = TMath::ChisquareQuantile(lProbPU,1);
  lChi2PU*=lChi2PU;
  return lChi2PU;
}
const std::vector<double> PuppiContainer::puppiWeights() {
    fPupParticles .resize(0);
    fWeights      .resize(0);
    fVals         .resize(0);
    for(int i0 = 0; i0 < fNAlgos; i0++) fPuppiAlgo[i0].reset();
    
    int lNMaxAlgo = 1;
    for(int i0 = 0; i0 < fNAlgos; i0++) lNMaxAlgo = TMath::Max(fPuppiAlgo[i0].numAlgos(),lNMaxAlgo);
    //Run through all compute mean and RMS
    int lNParticles    = fRecoParticles.size();
    for(int i0 = 0; i0 < lNMaxAlgo; i0++) { 
      getRMSAvg(i0,fPFParticles,fPFParticles,fChargedPV);
    }
    std::vector<double> pVals;
    for(int i0 = 0; i0 < lNParticles; i0++) {
      //Refresh
      pVals.clear();
      double pWeight = 1;
      //Get the Puppi Id and if ill defined move on
      int  pPupId   = getPuppiId(fRecoParticles[i0].pt,fRecoParticles[i0].eta);
      if(pPupId == -1) {
	fWeights .push_back(pWeight);
	continue;
      }
      // fill the p-values
      double pChi2   = 0;
      if(fUseDZ){ 
	//Compute an Experimental Puppi Weight with delta Z info (very simple example)
	pChi2 = getChi2FromdZ(fRecoParticles[i0].dZ);
	//Now make sure Neutrals are not set
	if(fRecoParticles[i0].pfType > 3) pChi2 = 0;
      }
      //Fill and compute the PuppiWeight
      int lNAlgos = fPuppiAlgo[pPupId].numAlgos();
      for(int i1 = 0; i1 < lNAlgos; i1++) pVals.push_back(fVals[lNParticles*i1+i0]);
      pWeight = fPuppiAlgo[pPupId].compute(pVals,pChi2);
      //Apply the CHS weights
      if(fPFParticles[i0].user_index() == 2 && fApplyCHS ) pWeight = 1;
      if(fPFParticles[i0].user_index() == 3 && fApplyCHS ) pWeight = 0;
      //Basic Weight Checks
      if(std::isnan(pWeight)) std::cerr << "====> Weight is nan  : pt " << fRecoParticles[i0].pt << " -- eta : " << fRecoParticles[i0].eta << " -- " << fVals[i0] << " -- " << lNAlgos << std::endl;
      //if(isnan(pWeight)) continue;
      //Basic Cuts      
      if(pWeight                         < fPuppiWeightCut) pWeight = 0;  //==> Elminate the low Weight stuff
      if(pWeight*fPFParticles[i0].pt()   < fNeutralMinPt && fRecoParticles[i0].pfType > 3 ) pWeight =0;  //threshold cut on the neutral Pt
      fWeights .push_back(pWeight);
      //Now get rid of the thrown out weights for the particle collection
      if(pWeight == 0) continue;
      //Produce
      PseudoJet curjet( pWeight*fPFParticles[i0].px(), pWeight*fPFParticles[i0].py(), pWeight*fPFParticles[i0].pz(), pWeight*fPFParticles[i0].e());
      curjet.set_user_index(i0);//fRecoParticles[i0].id);
      fPupParticles.push_back(curjet);
     }
    return fWeights;
}


