void format(TProfile *iProfile,int iColor) { 
  iProfile->SetLineStyle(kSolid);
  iProfile->SetMarkerStyle(kFullCircle);
  iProfile->SetLineColor(iColor);
  iProfile->SetMarkerColor(iColor);
}
TProfile** plotVars(std::string iLabel,std::string iCut,TTree *iTree,int iColor0,int iColor1) { 
  TProfile **lP = new TProfile*[2];
  lP[0] = new TProfile(("a0"+iLabel).c_str(),("a0"+iLabel).c_str(),40,-5.,5.);
  lP[1] = new TProfile(("a1"+iLabel).c_str(),("a1"+iLabel).c_str(),40,-5.,5.);
  format(lP[0],iColor0);
  format(lP[1],iColor1);
  std::string lCut = "pt > 1.0 && "+iCut;
  //iTree->Draw(("(alpha-alpha_med)/alpha_rms:eta>>a0"+iLabel).c_str(),lCut.c_str());
  //iTree->Draw(("(alpha-alpha_med)/alpha_rms:eta>>a1"+iLabel).c_str(),(lCut+" && genpt > 3.0").c_str());
  iTree->Draw(("weight:eta>>a0"+iLabel).c_str(),lCut.c_str());
  iTree->Draw(("weight:eta>>a1"+iLabel).c_str(),(lCut+" && genpt > 3.0").c_str());
  return lP;
}
void plotAlpha() { 
  TFile *lFile = new TFile("PuppiAlpha.root");
  TTree *lTree = (TTree*) lFile->FindObjectAny("alpha");
  
  TProfile **lP0 = plotVars("NH","pftype == 4",lTree,kBlack ,kRed);
  TProfile **lP1 = plotVars("GH","pftype == 5",lTree,kBlue  ,kOrange);
  TProfile **lP2 = plotVars("EH","pftype == 6",lTree,kGray  ,kRed+2);
  TProfile **lP3 = plotVars("FH","pftype == 7",lTree,kBlue+2,kOrange+2);
  //TProfile **lP2 = plotVars("CH","pftype == 1",lTree,kSpring,kMagenta);
  
  lP0[0]->GetYaxis()->SetRangeUser(-1,5);
  lP0[0]->Draw();
  for(int i0 = 0; i0 < 2; i0++) { 
    lP0[i0]->Draw("sames");
    lP1[i0]->Draw("sames");
    lP2[i0]->Draw("sames");
    lP3[i0]->Draw("sames");
  }
}
