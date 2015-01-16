import FWCore.ParameterSet.Config as cms

process = cms.Process('TestPuppi')
process.load('Configuration/StandardSequences/Services_cff')
process.load('FWCore/MessageService/MessageLogger_cfi')
process.load('Configuration/StandardSequences/FrontierConditions_GlobalTag_cff')

process.load("RecoMET.METProducers.PFMET_cfi")
process.pfMetPuppi     = process.pfMet.clone()
process.pfMetPuppi.src = cms.InputTag('puppi','Puppi')

process.MessageLogger.cerr.FwkReport.reportEvery = 10
process.GlobalTag.globaltag = 'START53_V7G::All'

process.load('Dummy/Puppi/Puppi_cff')   


process.isomuons = cms.EDFilter(
    "MuonSelector",
    src = cms.InputTag('muons'),
    cut = cms.string(    "(isTrackerMuon) && abs(eta) < 2.5 && pt > 20"
                         "&& isPFMuon"+
                         "&& globalTrack.isNonnull"+
                         "&& innerTrack.hitPattern.numberOfValidPixelHits > 0"+
                         "&& innerTrack.normalizedChi2 < 10"+
                         "&& numberOfMatches > 0"+
                         "&& innerTrack.hitPattern.numberOfValidTrackerHits>5"+
                         "&& globalTrack.hitPattern.numberOfValidHits>0"+
                         "&& (pfIsolationR03.sumChargedHadronPt+pfIsolationR03.sumNeutralHadronEt+pfIsolationR03.sumPhotonEt)/pt < 0.3"+
                         "&& abs(innerTrack().dxy)<2.0"
                         ),
    filter = cms.bool(True)
    )
process.dimuonsFilter = cms.EDFilter("CandViewCountFilter",
                                     src = cms.InputTag("isomuons"),
                                     minNumber = cms.uint32(2)
                                     )

process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(-1) )
process.source = cms.Source("PoolSource",
  fileNames  = cms.untracked.vstring('/store/relval/CMSSW_7_0_7/RelValZMM_13/GEN-SIM-RECO/PUpmx50ns_PLS170_V6AN1-v1/00000/D82DBE87-A11D-E411-989A-02163E00FEC4.root')
                            #fileNames  = cms.untracked.vstring('/store/relval/CMSSW_7_2_0_pre6/RelValZMM_13/GEN-SIM-RECO/PU50ns_PRE_LS172_V12-v1/00000/905DBDE6-7242-E411-9EC6-002618943950.root')
  #fileNames  = cms.untracked.vstring('file:/tmp/pharris/RSGravitonToWW_kMpl01_M_3000_Tune4C_13TeV_pythia8_PU_S14_PAT.root')
 # fileNames  = cms.untracked.vstring('root://cmsxrootd-site.fnal.gov//store/results/top/StoreResults/TTJets_MSDecaysCKM_central_Tune4C_13TeV-madgraph-tauola/USER/Spring14dr_PU_S14_POSTLS170_V6AN1_miniAOD706p1_814812ec83fce2f620905d2bb30e9100-v2/00000/0012F41F-FA17-E411-A1FF-0025905A48B2.root')
)
process.source.inputCommands = cms.untracked.vstring("keep *",
                                                     "drop *_MEtoEDMConverter_*_*")

process.options = cms.untracked.PSet(
  wantSummary = cms.untracked.bool(True),
  Rethrow     = cms.untracked.vstring('ProductNotFound'),
  fileMode    = cms.untracked.string('NOMERGE')
)


process.muonSequence  = cms.Sequence(process.isomuons*process.dimuonsFilter)
process.puppiSequence = cms.Sequence(process.puppi*process.pfMetPuppi)
process.p = cms.Path(process.muonSequence*process.puppiSequence)
process.output = cms.OutputModule("PoolOutputModule",                                                                                                                                                     
                                  outputCommands = cms.untracked.vstring('drop *','drop *_*_Cleaned_*','keep *_puppi_*_*','keep *_pfMet*_*_*'),
                                  fileName       = cms.untracked.string ("Output.root")                                                                                                                   
)
# schedule definition                                                                                                       
process.outpath  = cms.EndPath(process.output) 
