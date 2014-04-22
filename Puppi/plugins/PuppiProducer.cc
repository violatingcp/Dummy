// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrackFwd.h"
//Main File
#include "fastjet/PseudoJet.hh"
#include "Dummy/Puppi/plugins/PuppiProducer.h"

// ------------------------------------------------------------------------------------------
PuppiProducer::PuppiProducer(const edm::ParameterSet& iConfig) {
  fPuppiName = iConfig.getUntrackedParameter<std::string>("PuppiName");
  fUseDZ     = iConfig.getUntrackedParameter<bool>("UseDeltaZCut");
  fDZCut     = iConfig.getUntrackedParameter<double>("DeltaZCut");
  fPuppiContainer = new PuppiContainer(iConfig);
  fPFName    = iConfig.getUntrackedParameter<std::string>("candName"  ,"particleFlow");
  fPVName    = iConfig.getUntrackedParameter<std::string>("vertexName","offlinePrimaryVertices");
}
// ------------------------------------------------------------------------------------------
PuppiProducer::~PuppiProducer(){
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {

  // Get PFCandidate Collection
  edm::Handle<reco::PFCandidateCollection> hPFProduct;
  iEvent.getByLabel(fPFName,hPFProduct);
  assert(hPFProduct.isValid());
  const reco::PFCandidateCollection *PFCol = hPFProduct.product();

  // Get vertex collection w/PV as the first entry?
  edm::Handle<reco::VertexCollection> hVertexProduct;
  iEvent.getByLabel(fPVName,hVertexProduct);
  assert(hVertexProduct.isValid());
  const reco::VertexCollection *pvCol = hVertexProduct.product();

  //Fill the reco objects
  fRecoObjCollection.clear();
  for(reco::PFCandidateCollection::const_iterator itPF = PFCol->begin(); itPF!=PFCol->end(); itPF++) {
    RecoObj pReco;
    pReco.pt  = itPF->pt();
    pReco.eta = itPF->eta();
    pReco.phi = itPF->phi();
    pReco.m   = itPF->mass();
    
    const reco::Vertex *closestVtx = 0;
    double pDZ    = 0; 
    double pD0    = 0; 
    int    pVtxId = 0; 
    bool lFirst = true;
    for(reco::VertexCollection::const_iterator iV = pvCol->begin(); iV!=pvCol->end(); ++iV) {
      if(lFirst) { 
	if      ( itPF->trackRef().isNonnull()    ) pDZ = itPF->trackRef()   ->dz(iV->position());
	else if ( itPF->gsfTrackRef().isNonnull() ) pDZ = itPF->gsfTrackRef()->dz(iV->position());
	if      ( itPF->trackRef().isNonnull()    ) pD0 = itPF->trackRef()   ->d0();
	else if ( itPF->gsfTrackRef().isNonnull() ) pD0 = itPF->gsfTrackRef()->d0();
	lFirst = false;
      }
      if(iV->trackWeight(itPF->trackRef())>0) {
	closestVtx  = &(*iV);
	break;
      }
      pVtxId++;
    }
    pReco.dZ      = pDZ;
    pReco.d0      = pD0;
    if(closestVtx == 0) pReco.vtxId = -1;
    if(closestVtx != 0) pReco.vtxId = pVtxId;
    if(closestVtx != 0) pReco.vtxChi2 = closestVtx->trackWeight(itPF->trackRef());
    //Set the id for Puppi Algo   (1- Neutral, 2-Charged PV, 3-Charged PU)
    pReco.id       = 1; 
    if(closestVtx != 0 && pVtxId == 0)                   pReco.id = 2;
    if(closestVtx != 0 && pVtxId >  0)                   pReco.id = 3;
    //Add a dZ cut if wanted (this helps)
    if(fUseDZ && closestVtx == 0 && (fabs(pDZ) < fDZCut)) pReco.id = 2; 
    if(fUseDZ && closestVtx == 0 && (fabs(pDZ) > fDZCut)) pReco.id = 3; 
    fRecoObjCollection.push_back(pReco);
  }
  fPuppiContainer->initialize(fRecoObjCollection);

  //Compute the weights and the candidates
  const std::vector<double> lWeights = fPuppiContainer->puppiWeights();
  //Fill it into the event
  std::auto_ptr<edm::ValueMap<float> > lPupOut(new edm::ValueMap<float>());
  edm::ValueMap<float>::Filler  lPupFiller(*lPupOut);
  lPupFiller.insert(hPFProduct,lWeights.begin(),lWeights.end());
  lPupFiller.fill();  
  iEvent.put(lPupOut,"PuppiWeights");

  //Fill a new PF Candidate Collection
  const std::vector<fastjet::PseudoJet> lCandidates = fPuppiContainer->puppiParticles();
  fPuppiCandidates.reset( new reco::PFCandidateCollection );    
  for(unsigned int i0 = 0; i0 < lCandidates.size(); i0++) {
    reco::PFCandidate pCand(PFCol->at(lCandidates[i0].user_index()));
    LorentzVector pVec; pVec.SetPxPyPzE(lCandidates[i0].px(),lCandidates[i0].py(),lCandidates[i0].pz(),lCandidates[i0].E());
    pCand.setP4(pVec);
    fPuppiCandidates->push_back(pCand);
    std::cout << "===> Check " << lCandidates[i0].pt() << " -- " << pCand.pt() << std::endl;
  }
  iEvent.put(fPuppiCandidates,fPuppiName);
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::beginJob() {
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::endJob() {
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::beginRun(edm::Run&, edm::EventSetup const&) {
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::endRun(edm::Run&, edm::EventSetup const&) {
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::beginLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&) {
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::endLuminosityBlock(edm::LuminosityBlock&, edm::EventSetup const&) {
}
// ------------------------------------------------------------------------------------------
void PuppiProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
	//The following says we do not know what parameters are allowed so do no validation
	// Please change this to state exactly what you do use, even if it is no parameters
	edm::ParameterSetDescription desc;
	desc.setUnknown();
	descriptions.addDefault(desc);
}
//define this as a plug-in
DEFINE_FWK_MODULE(PuppiProducer);
