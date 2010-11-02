#include "app.h"
#include "UserSkins.h"

/*	THIS IS OUR MAIN "START UP" FILE.
	App.cpp creates our wxApp class object.
	the wxApp initiates our program (takes over the role of main())
	When our wxApp loads,  it creates our ModelViewer class object,  
	which is a wxWindow.  From there ModelViewer object then creates
	our menu bar, character control, view control, filetree control, 
	animation control, and the canvas control (opengl).  Once those 
	controls are created it then loads saved variables from the config.ini
	file.  Then it proceeds	to create and open the MPQ archives,  creating
	a file list of the contents from all files within all of the opened mpq archives.

	I hope this gives some insight into the "program flow".
*/
/*
#ifdef _DEBUG
	#define new DEBUG_CLIENTBLOCK
#endif
*/

// tell wxwidgets which class is our app
IMPLEMENT_APP(WowModelViewApp)

//#include "globalvars.h"

bool WowModelViewApp::OnInit()
{
	frame = NULL;
	LogFile = NULL;


	// Error & Logging settings
#ifndef _DEBUG
	#if wxUSE_ON_FATAL_EXCEPTION
		wxHandleFatalExceptions(true);
	#endif
#endif

	wxFileName fname(argv[0]);
	wxString userPath = fname.GetPath(wxPATH_GET_VOLUME)+SLASH+wxT("userSettings");
	wxFileName::Mkdir(userPath, 0777, wxPATH_MKDIR_FULL);

	// set the log file path.
	wxString logPath = userPath+SLASH+wxT("log.txt");

	LogFile = fopen(logPath.mb_str(), "w+");
	if (LogFile) {
		wxLog *logger = new wxLogStderr(LogFile);
		delete wxLog::SetActiveTarget(logger);
		wxLog::SetVerbose(false);
	}

	// Application Info
	SetVendorName(_T("WoWModelViewer"));
	SetAppName(_T("WoWModelViewer"));

	// Just a little header to start off the log file.
	wxLogMessage(wxString(_T("Starting:\n") APP_TITLE _T(" ") APP_VERSION _T(" ") APP_PLATFORM APP_ISDEBUG _T("\n\n")));

	// set the config file path.
	cfgPath = userPath+SLASH+wxT("Config.ini");

	bool loadfail = LoadSettings();
	if (loadfail == true){
		return false;
	}

	// Load user skins 
	gUserSkins.LoadFile(userPath + SLASH + _T("Skins.txt"));
	if (!gUserSkins.Loaded())
		wxLogMessage(_T("Warning: Failed to load user skins"));


#ifdef _WINDOWS
	// This chunk of code is all related to locale translation (if a translation is available).
	// Only use locale for non-english?
	wxString fn = _T("mo");
	fn = fn+SLASH+locales[interfaceID]+_T(".mo");
	if (wxFileExists(fn))
	{
		locale.Init(langIds[interfaceID], wxLOCALE_CONV_ENCODING);
		
		wxLocale::AddCatalogLookupPathPrefix(_T("mo"));
		//wxLocale::AddCatalogLookupPathPrefix(_T(".."));

		//locale.AddCatalog(_T("wowmodelview")); // Initialize the catalogs we'll be using
		locale.AddCatalog(locales[interfaceID]);
	}
#endif

	// Now create our main frame.
    frame = new ModelViewer();
    
	if (!frame) {
		//this->Close();
		return false;
	}
	
	SetTopWindow(frame);

	// Set the icon, different source location for the icon under Linux & Mac
	wxIcon icon;
#if defined (_WINDOWS)
	if (icon.LoadFile(_T("mainicon"),wxBITMAP_TYPE_ICO_RESOURCE) == false)
		wxMessageBox(_T("Failed to load Icon"),_T("Failure"));
#elif defined (_LINUX)
	// This probably needs to be fixed...
	//if (icon.LoadFile(_T("../bin_support/icon/wmv_xpm")) == false)
	//	wxMessageBox(_T("Failed to load Icon"),_T("Failure"));
#elif defined (_MAC)
	// Dunno what to do about Macs...
	//if (icon.LoadFile(_T("../bin_support/icon/wmv.icns")) == false)
	//	wxMessageBox(_T("Failed to load Icon"),_T("Failure"));
#endif
	frame->SetIcon(icon);
	// --

	// Point our global vars at the correct memory location
	g_modelViewer = frame;
	g_canvas = frame->canvas;
	g_animControl = frame->animControl;
	g_charControl = frame->charControl;
	g_fileControl = frame->fileControl;

	frame->interfaceManager.Update();

	if (frame->canvas) {
		frame->canvas->Show(true);
		
		if (!frame->canvas->init)
			frame->canvas->InitGL();

		if (frame->lightControl)
			frame->lightControl->UpdateGL();
	}
	// --
	

	// TODO: Improve this feature and expand on it.
	// Command arguments
	wxString cmd;
	for (int i=0; i<argc; i++) {
		cmd = argv[i];

		if (cmd == _T("-m")) {
			if (i+1 < argc) {
				i++;
				wxString fn(argv[i]);

				// Error check
				if (fn.Last() != '2') // Its not an M2 file, exit
					break;

				// Load the model
				frame->LoadModel(fn);
			}
		} else if (cmd == _T("-mo")) {
			if (i+1 < argc) {
				i++;
				wxString fn(argv[i]);

				if (fn.Last() != '2') // Its not an M2 file, exit
					break;

				// If its a character model, give it some skin.
				// Load the model
				frame->LoadModel(fn);

				// Output the screenshot
				fn = fn.AfterLast('\\');
				fn = fn.BeforeLast('.');
				fn.Prepend(_T("ss_"));
				fn.Append(_T(".png"));
				frame->canvas->Screenshot(fn);
			}
		} else {
			wxString tmp = cmd.AfterLast('.');
			if (!tmp.IsNull() && !tmp.IsEmpty() && tmp.IsSameAs(wxT("chr"), false))
				frame->LoadChar(cmd);
		}
	}
	// -------

	// Load previously saved layout
	frame->LoadLayout();

	wxLogMessage(_T("WoW Model Viewer successfully loaded!\n----\n"));
	
	return true;
}

void WowModelViewApp::OnFatalException()
{
	//wxApp::SetExitOnFrameDelete(false);

	/*
	wxDebugReport report;
    wxDebugReportPreviewStd preview;

	report.AddAll(wxDebugReport::Context_Exception);

    if (preview.Show(report))
        report.Process();
	*/

	if (frame != NULL) {
		frame->Destroy();
		frame = NULL;
	}
}

int WowModelViewApp::OnExit()
{
	SaveSettings();
	
	//if (frame != NULL)
	//	frame->Destroy();

//#ifdef _DEBUG
	//delete wxLog::SetActiveTarget(NULL);
	if (LogFile) {
		fclose(LogFile);
		//wxDELETE(LogFile);
		LogFile = NULL;
	}
//#endif

	CleanUp();

	//_CrtMemDumpAllObjectsSince( NULL );

	return 0;
}

/*
void WowModelViewApp::HandleEvent(wxEvtHandler *handler, wxEventFunction func, wxEvent& event) const 
{ 
    try 
    {        
        HandleEvent(handler, func, event); 
	} 
	catch(...) 
	{ 
		wxMessageBox(_T("An error occured while handling an application event."), _T("Execption in event handling"), wxOK | wxICON_ERROR); 
		throw; 
	} 
}
*/

void WowModelViewApp::OnUnhandledException() 
{ 
    //wxMessageBox(_T("An unhandled exception was caught, the program will now terminate."), _T("Unhandled Exception"), wxOK | wxICON_ERROR); 
	wxLogFatalError(_T("An unhandled exception error has occured."));
}

namespace {
	long traverseLocaleMPQs(const wxString locales[], size_t localeCount, const wxString localeArchives[], size_t archiveCount, const wxString& gamePath)
	{
		long lngID = -1;

		for (size_t i = 0; i < localeCount; i++) {
			if (locales[i].IsEmpty())
				continue;
			wxString localePath = gamePath;

			localePath.Append(locales[i]);
			localePath.Append(_T("/"));
			if (wxDir::Exists(localePath)) {
				wxArrayString localeMpqs;
				wxDir::GetAllFiles(localePath, &localeMpqs, wxEmptyString, wxDIR_FILES);

				for (size_t j = 0; j < archiveCount; j++) {
					for (size_t k = 0; k < localeMpqs.size(); k++) {
						wxString baseName = wxFileName(localeMpqs[k]).GetFullName();
						wxString neededMpq = wxString::Format(localeArchives[j], locales[i].c_str());

						if(baseName.CmpNoCase(neededMpq) == 0) {
							mpqArchives.Add(localeMpqs[k]);
						}
					}
				}

				lngID = (long)i;
				return lngID;
			}
		}

		return lngID;
	}
}

void searchMPQs()
{
	if (mpqArchives.GetCount() > 0)
		return;
	//enUS(enGB), koKR, frFR, deDE, zhCN, zhTW, esES, ruRU
	const int localeSets = 8;
	const wxString locales[] = {
		// sets 0
		_T("enUS"), _T("koKR"), _T("frFR"), _T("deDE"), 
		_T("zhCN"), _T("zhTW"), _T("esES"), _T("ruRU"),
		// sets 1
		_T("enGB"), wxEmptyString, wxEmptyString, wxEmptyString, 
		_T("enCN"), _T("enTW"), _T("esMX"), wxEmptyString
		};

	const wxString defaultArchives[] = {_T("patch-9.mpq"),_T("patch-8.mpq"),_T("patch-7.mpq"),_T("patch-6.mpq"),
		_T("patch-5.mpq"),_T("patch-4.mpq"),_T("patch-3.mpq"),_T("patch-2.mpq"),_T("patch.mpq"),_T("alternate.mpq"),
		_T("expansion3.mpq"),_T("expansion2.mpq"),_T("expansion1.mpq"),_T("lichking.mpq"),_T("expansion.mpq"),
		_T("world.mpq"),_T("sound.mpq"),_T("art.mpq"),_T("common-3.mpq"),_T("common-2.mpq"), _T("common.mpq")};
	const wxString localeArchives[] = {_T("patch-%s-9.mpq"),_T("patch-%s-8.mpq"),_T("patch-%s-7.mpq"),
		_T("patch-%s-6.mpq"),_T("patch-%s-5.mpq"),_T("patch-%s-4.mpq"),_T("patch-%s-3.mpq"), _T("patch-%s-2.mpq"), 
		_T("patch-%s.mpq"), _T("expansion3-locale-%s.mpq"), _T("expansion2-locale-%s.mpq"), 
		_T("expansion1-locale-%s.mpq"), _T("lichking-locale-%s.mpq"), _T("expansion-locale-%s.mpq"), 
		_T("locale-%s.mpq"), _T("base-%s.mpq")};

	// select avaiable locales
	wxArrayString avaiLocales;
	for (size_t i = 0; i < WXSIZEOF(locales); i++) {
		if (locales[i].IsEmpty())
			continue;
		wxString localePath = gamePath;

		localePath.Append(locales[i]);
		localePath.Append(_T("/"));
		if (wxDir::Exists(localePath)) {
			avaiLocales.Add(locales[i]);
		}
	}
	wxString sLocale;
	if (avaiLocales.size() == 1) // only 1 locale
		sLocale = avaiLocales[0];
	else
		sLocale = wxGetSingleChoice(_T("Please select a Locale:"), _T("Locale"), avaiLocales);

	// search Partial MPQs
	wxArrayString baseMpqs;
	wxDir::GetAllFiles(gamePath, &baseMpqs, wxEmptyString, wxDIR_FILES);
	for (size_t j = 0; j < baseMpqs.size(); j++) {
		if (baseMpqs[j].Contains(_T("oldworld")))
			continue;
		wxString baseName = wxFileName(baseMpqs[j]).GetFullName();
		wxString cmpName = _T("wow-update-");
		if (baseName.StartsWith(cmpName) && baseName.AfterLast('.').CmpNoCase(_T("mpq")) == 0) {
			bool bFound = false;
			for(size_t i = 0; i<mpqArchives.size(); i++) {
				if (!mpqArchives[i].AfterLast(SLASH).StartsWith(cmpName))
					continue;
				int ver = wxAtoi(mpqArchives[i].BeforeLast('.').AfterLast('-'));
				int bver = wxAtoi(baseName.BeforeLast('.').AfterLast('-'));
				if (bver > ver) {
					mpqArchives.Insert(baseMpqs[j], i);
					bFound = true;
					break;
				}		
			}
			if (bFound == false)
				mpqArchives.Add(baseMpqs[j]);

			wxLogMessage(_T("- Found Partial MPQ archive: %s"), baseMpqs[j].c_str());
		}
	}

	// search patch-base MPQs
	wxArrayString baseCacheMpqs;
	wxDir::GetAllFiles(gamePath+_T("Cache"), &baseCacheMpqs, wxEmptyString, wxDIR_FILES);
	for (size_t j = 0; j < baseCacheMpqs.size(); j++) {
		if (baseCacheMpqs[j].Contains(_T("oldworld")))
			continue;
		wxString baseName = baseCacheMpqs[j];
		wxString fullName = wxFileName(baseName).GetFullName();
		wxString cmpName = _T("patch-base-");
		if (fullName.StartsWith(cmpName) && fullName.AfterLast('.').CmpNoCase(_T("mpq")) == 0) {
			bool bFound = false;
			for(size_t i = 0; i<mpqArchives.size(); i++) {
				if (!mpqArchives[i].AfterLast(SLASH).StartsWith(cmpName))
					continue;
				int ver = wxAtoi(mpqArchives[i].BeforeLast('.').AfterLast('-'));
				int bver = wxAtoi(fullName.BeforeLast('.').AfterLast('-'));
				if (bver > ver) {
#if 1 // Use lastest archive only
					mpqArchives[i] = baseName;
#else
					mpqArchives.Insert(baseName, i);
#endif
					bFound = true;
					break;
				}		
			}
			if (bFound == false)
				mpqArchives.Add(baseName);

			wxLogMessage(_T("- Found Patch Base MPQ archive: %s"), baseName.c_str());
		}
	}
	baseCacheMpqs.Clear();

	// search base cache locale MPQs
	wxArrayString baseCacheLocaleMpqs;
	wxDir::GetAllFiles(gamePath+_T("Cache")+SLASH+sLocale, &baseCacheLocaleMpqs, wxEmptyString, wxDIR_FILES);
	for (size_t j = 0; j < baseCacheLocaleMpqs.size(); j++) {
		if (baseCacheLocaleMpqs[j].Contains(_T("oldworld")))
			continue;
		wxString baseName = baseCacheLocaleMpqs[j];
		wxString fullName = wxFileName(baseName).GetFullName();
		wxString cmpName = _T("patch-")+sLocale+_T("-");
		if (fullName.StartsWith(cmpName) && fullName.AfterLast('.').CmpNoCase(_T("mpq")) == 0) {
			bool bFound = false;
			for(size_t i = 0; i<mpqArchives.size(); i++) {
				if (!mpqArchives[i].AfterLast(SLASH).StartsWith(cmpName))
					continue;
				int ver = wxAtoi(mpqArchives[i].BeforeLast('.').AfterLast('-'));
				int bver = wxAtoi(fullName.BeforeLast('.').AfterLast('-'));
				if (bver > ver) {
#if 1 // Use lastest archive only
					mpqArchives[i] = baseName;
#else
					mpqArchives.Insert(baseName, i);
#endif
					bFound = true;
					break;
				}		
			}
			if (bFound == false)
				mpqArchives.Add(baseName);

			wxLogMessage(_T("- Found Patch Base Locale MPQ archive: %s"), baseName.c_str());
		}
	}
	baseCacheLocaleMpqs.Clear();

	// default archives
	for (size_t i = 0; i < WXSIZEOF(defaultArchives); i++) {
		//wxLogMessage(_T("Searching for MPQ archive %s..."), defaultArchives[i].c_str());

		for (size_t j = 0; j < baseMpqs.size(); j++) {
			wxString baseName = wxFileName(baseMpqs[j]).GetFullName();
			if(baseName.CmpNoCase(defaultArchives[i]) == 0) {
				mpqArchives.Add(baseMpqs[j]);

				wxLogMessage(_T("- Found MPQ archive: %s"), baseMpqs[j].c_str());
				if (baseName.CmpNoCase(_T("alternate.mpq")))
					bAlternate = true;
			}
		}
	}

	// add locale files
	for (size_t i = 0; i < WXSIZEOF(locales); i++) {
		if (locales[i] == sLocale) {
			wxString localePath = gamePath;

			localePath.Append(locales[i]);
			localePath.Append(_T("/"));
			if (wxDir::Exists(localePath)) {
				wxArrayString localeMpqs;
				wxDir::GetAllFiles(localePath, &localeMpqs, wxEmptyString, wxDIR_FILES);

				for (size_t j = 0; j < WXSIZEOF(localeArchives); j++) {
					for (size_t k = 0; k < localeMpqs.size(); k++) {
						wxString baseName = wxFileName(localeMpqs[k]).GetFullName();
						wxString neededMpq = wxString::Format(localeArchives[j], locales[i].c_str());

						if(baseName.CmpNoCase(neededMpq) == 0) {
							mpqArchives.Add(localeMpqs[k]);
						}
					}
				}
			}

			langID = i % localeSets;
			break;
		}
	}

	if (langID == -1) {
		langID = traverseLocaleMPQs(locales, WXSIZEOF(locales), localeArchives, WXSIZEOF(localeArchives), gamePath);
		if (langID != -1)
			langID = langID % localeSets;
	}

}

bool WowModelViewApp::LoadSettings()
{
	wxString tmp;
	// Application Config Settings
	wxFileConfig *pConfig = new wxFileConfig(_T("Global"), wxEmptyString, cfgPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
	
	// Graphic / Video display settings
	pConfig->SetPath(_T("/Graphics"));
	pConfig->Read(_T("FSAA"), &video.curCap.aaSamples, 0);
	pConfig->Read(_T("AccumulationBuffer"), &video.curCap.accum, 0);
	pConfig->Read(_T("AlphaBits"), &video.curCap.alpha, 0);
	pConfig->Read(_T("ColourBits"), &video.curCap.colour, 24);
	pConfig->Read(_T("DoubleBuffer"), (bool*)&video.curCap.doubleBuffer, 1);	// True
#ifdef _WINDOWS
	pConfig->Read(_T("HWAcceleration"), &video.curCap.hwAcc, WGL_FULL_ACCELERATION_ARB);
#endif
	pConfig->Read(_T("SampleBuffer"), (bool*)&video.curCap.sampleBuffer, 0);	// False
	pConfig->Read(_T("StencilBuffer"), &video.curCap.stencil, 0);
	pConfig->Read(_T("ZBuffer"), &video.curCap.zBuffer, 16);

	// Application locale info
	pConfig->SetPath(_T("/Locale"));
	pConfig->Read(_T("LanguageID"), &langID, -1);
	pConfig->Read(_T("InterfaceID"), &interfaceID, -1);

	// Application settings
	pConfig->SetPath(_T("/Settings"));
	pConfig->Read(_T("Path"), &gamePath, wxEmptyString);
	pConfig->Read(_T("TOCVersion"), &gameVersion, 0);

	pConfig->Read(_T("UseLocalFiles"), &useLocalFiles, false);
	pConfig->Read(_T("SSCounter"), &ssCounter, 100);
	//pConfig->Read(_T("AntiAlias"), &useAntiAlias, true);
	//pConfig->Read(_T("DisableHWAcc"), &disableHWAcc, false);
	pConfig->Read(_T("DefaultFormat"), &imgFormat, 0);

	pConfig->Read(_T("PerferedExporter"), &Perfered_Exporter, -1);
	pConfig->Read(_T("ModelExportInitOnly"), &modelExportInitOnly, true);
	pConfig->Read(_T("ModelExportPreserveDirs"), &modelExport_PreserveDir, true);
	pConfig->Read(_T("ModelExportUseWMVPosRot"), &modelExport_UseWMVPosRot, false);

	pConfig->Read(_T("ModelExportLWPreserveDirs"), &modelExport_LW_PreserveDir, true);
	pConfig->Read(_T("ModelExportLWExportLights"), &modelExport_LW_ExportLights, true);
	pConfig->Read(_T("ModelExportLWExportDoodads"), &modelExport_LW_ExportDoodads, true);
	pConfig->Read(_T("ModelExportLWDoodadsAs"), &modelExport_LW_DoodadsAs, 0);

	pConfig->Read(_T("ModelExportM3BoundScale"), &tmp, _T("0.5"));
	modelExport_M3_BoundScale = wxAtof(tmp);
	pConfig->Read(_T("ModelExportM3SphereScale"), &tmp, _T("0.5"));
	modelExport_M3_SphereScale = wxAtof(tmp);
	pConfig->Read(_T("ModelExportM3TexturePath"), &modelExport_M3_TexturePath, wxEmptyString);


	// Data path and mpq archive stuff
	wxString archives;
	pConfig->Read(_T("MPQFiles"), &archives);
	
	wxStringTokenizer strToken(archives, _T(";"), wxTOKEN_DEFAULT);
	while (strToken.HasMoreTokens()) {
		mpqArchives.Add(strToken.GetNextToken());
	}

	if (gamePath.IsEmpty() || !wxDirExists(gamePath)) {
		getGamePath();
		mpqArchives.Clear();
	}

	if (gamePath.Last() != SLASH)
		gamePath.Append(SLASH);

	if (mpqArchives.GetCount()==0) {
		searchMPQs();
	}

	// if we can't search any mpqs
	if (mpqArchives.GetCount()==0) {
		wxLogMessage(_T("World of Warcraft Data Directory Not Found. Returned GamePath: %s"),gamePath.c_str());
		wxMessageDialog *dial = new wxMessageDialog(NULL, wxT("Fatal Error: Could not find your World of Warcraft Data folder."), wxT("World of Warcraft Not Found"), wxOK | wxICON_ERROR);
		dial->ShowModal();
		return true;
	}

	// Clear our ini file config object
	wxDELETE( pConfig );

    if (langID == -1) {
        // the arrays should be in sync
        wxCOMPILE_TIME_ASSERT(WXSIZEOF(langNames) == WXSIZEOF(langIds), LangArraysMismatch);
        langID = wxGetSingleChoiceIndex(_T("Please select a language:"), _T("Language"), WXSIZEOF(langNames), langNames);

		SaveSettings();
	}

	// initial interfaceID as langID
	if (interfaceID == -1)
		interfaceID = langID;

	// initial langOffset
	if (langOffset == -1) {
		// gameVersion 40000 remove all other language strings
		if (gameVersion == 40000)
			langOffset = 0;
		else
			langOffset = langID;
	}
	return false;
}

void WowModelViewApp::SaveSettings()
{
	// Application Config Settings
	wxFileConfig *pConfig = new wxFileConfig(_T("Global"), wxEmptyString, cfgPath, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
	
	pConfig->SetPath(_T("/Locale"));
	pConfig->Write(_T("LanguageID"), langID);
	pConfig->Write(_T("InterfaceID"), interfaceID);

	pConfig->SetPath(_T("/Settings"));
	pConfig->Write(_T("Path"), gamePath);
	pConfig->Write(_T("TOCVersion"), gameVersion);
	pConfig->Write(_T("UseLocalFiles"), useLocalFiles);
	pConfig->Write(_T("SSCounter"), ssCounter);
	//pConfig->Write(_T("AntiAlias"), useAntiAlias);
	//pConfig->Write(_T("DisableHWAcc"), disableHWAcc);
	pConfig->Write(_T("DefaultFormat"), imgFormat);

	pConfig->Write(_T("PerferedExporter"), Perfered_Exporter);
	pConfig->Write(_T("ModelExportInitOnly"), modelExportInitOnly);
	pConfig->Write(_T("ModelExportPreserveDirs"), modelExport_PreserveDir);
	pConfig->Write(_T("ModelExportUseWMVPosRot"), modelExport_UseWMVPosRot);

	pConfig->Write(_T("ModelExportLWPreserveDirs"), modelExport_LW_PreserveDir);
	pConfig->Write(_T("ModelExportLWExportLights"), modelExport_LW_ExportLights);
	pConfig->Write(_T("ModelExportLWExportDoodads"), modelExport_LW_ExportDoodads);
	pConfig->Write(_T("ModelExportLWDoodadsAs"), modelExport_LW_DoodadsAs);

	pConfig->Write(_T("ModelExportM3BoundScale"), wxString::Format(_T("%0.2f"), modelExport_M3_BoundScale));
	pConfig->Write(_T("ModelExportM3SphereScale"), wxString::Format(_T("%0.2f"), modelExport_M3_SphereScale));
	pConfig->Write(_T("ModelExportM3TexturePath"), modelExport_M3_TexturePath);

	wxString archives;

	for (size_t i=0; i<mpqArchives.GetCount(); i++) {
		archives.Append(mpqArchives[i]);
		archives.Append(_T(";"));
	}

	pConfig->Write(_T("MPQFiles"), archives);

	// Clear our ini file config object
	wxDELETE( pConfig );
}


