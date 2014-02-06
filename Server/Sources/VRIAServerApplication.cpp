/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#include "headers4d.h"
#include "DB4D/Headers/DB4D.h"
// db4d bag keys, to be included once and only once per project
#include "DB4D/Headers/DB4DBagKeys.h"
#include "JavaScript/VJavaScript.h"
#include "VRIAServerComponentBridge.h"
#include "VSolution.h"
#include "VRIAServerSolution.h"
#include "VRIAServerTools.h"
#include "VSolutionStartupParameters.h"
#include "Language Syntax/CLanguageSyntax.h"
#include "VRIAServerApplication.h"
#include "ServerNet/VServerNet.h"
#include "VRIAServerProject.h"
#include "Kernel/Sources/VChecksumMD5.h"
#include "Kernel/Sources/MurmurHash.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "VRIAServerJSAPI.h"
#include "VRIAServerJSCore.h"
#include "VRIAServerSupervisor.h"
#include "VRIAServerProgressIndicator.h"
#include "VProject.h"



#if VERSIONMAC
#include "AuthorizationHelpers.h"
#endif


USING_TOOLBOX_NAMESPACE


const uLONG	kMIN_DATA_CACHE_FLUSH_DELAY			= 60000;	// minimum delay is 1 min
const uLONG kDEFAULT_DATA_CACHE_FLUSH_DELAY		= 900000;	// default delay is 15 min


//** Change those hardcoded constants.

#define DEFAULT_NAME			"noname"

#define	DEFAULT_SERVICE_NAME 	"_http._tcp"

#define DEFAULT_PORT			(80)



// ----------------------------------------------------------------------------


namespace ServerStartupParametersKeys
{
	CREATE_BAGKEY_NO_DEFAULT( defaultAdministratorHttpPort, XBOX::VLong);
	CREATE_BAGKEY_NO_DEFAULT( defaultAdministratorSSLPort, XBOX::VLong);
	CREATE_BAGKEY_NO_DEFAULT( quitServerWhenSolutionIsClosed, XBOX::VBoolean);
	CREATE_BAGKEY_NO_DEFAULT( netDump, XBOX::VBoolean);
	CREATE_BAGKEY_NO_DEFAULT( debuggingAuthorized, XBOX::VBoolean);
	CREATE_BAGKEY_NO_DEFAULT( debuggerType, XBOX::VLong);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( allowDebugAdministrator, XBOX::VBoolean, bool, false);	// sc 23/04/2013, by default, debugging in not allowed into the default solution
	CREATE_BAGKEY( adminLogin);
	CREATE_BAGKEY( adminPassword);
	CREATE_BAGKEY( jobID);
	CREATE_BAGKEY_WITH_DEFAULT_SCALAR( withSandboxedProject, XBOX::VBoolean, bool, false);
}


VRIAServerStartupParameters::VRIAServerStartupParameters()
: fSolutionFile(NULL), fJavaScriptFile(NULL)
{
}


VRIAServerStartupParameters::~VRIAServerStartupParameters()
{
	ReleaseRefCountable( &fSolutionFile);
	ReleaseRefCountable( &fJavaScriptFile);
}


void VRIAServerStartupParameters::SetSolutionToLaunch( XBOX::VFile* inSolutionFile)
{
	CopyRefCountable( &fSolutionFile, inSolutionFile);
}


XBOX::VFile* VRIAServerStartupParameters::GetSolutionToLaunch() const
{
	return fSolutionFile;
}


void VRIAServerStartupParameters::SetAdministratorHttpPort( sLONG inPort)
{
	fBag.SetLong( ServerStartupParametersKeys::defaultAdministratorHttpPort, inPort);
}


bool VRIAServerStartupParameters::GetAdministratorHttpPort( sLONG& outPort) const
{
	return fBag.GetLong( ServerStartupParametersKeys::defaultAdministratorHttpPort, outPort);
}


void VRIAServerStartupParameters::SetAdministratorSSLPort( sLONG inPort)
{
	fBag.SetLong( ServerStartupParametersKeys::defaultAdministratorSSLPort, inPort);
}


bool VRIAServerStartupParameters::GetAdministratorSSLPort( sLONG& outPort) const
{
	return fBag.GetLong( ServerStartupParametersKeys::defaultAdministratorSSLPort, outPort);
}


void VRIAServerStartupParameters::SetQuitServerWhenSolutionIsClosed( bool inQuitServer)
{
	fBag.SetBool( ServerStartupParametersKeys::quitServerWhenSolutionIsClosed, inQuitServer);
}


bool VRIAServerStartupParameters::GetQuitServerWhenSolutionIsClosed( bool& outQuitServer)
{
	return fBag.GetBool( ServerStartupParametersKeys::quitServerWhenSolutionIsClosed, outQuitServer);
}


void VRIAServerStartupParameters::SetJavaScriptFileToExecute( XBOX::VFile* inJavaScriptFile)
{
	CopyRefCountable( &fJavaScriptFile, inJavaScriptFile);
}


XBOX::VFile* VRIAServerStartupParameters::GetJavaScriptFileToExecute() const
{
	return fJavaScriptFile;
}


void VRIAServerStartupParameters::SetNetDump(bool inWithServerNetDump)
{
	fBag.SetBool( ServerStartupParametersKeys::netDump, inWithServerNetDump);
}


bool VRIAServerStartupParameters::GetNetDump(bool& outWithServerNetDump)
{
	return fBag.GetBool( ServerStartupParametersKeys::netDump, outWithServerNetDump);
}


void VRIAServerStartupParameters::SetDebuggingAuthorized( bool inAutorized)
{
	fBag.SetBool( ServerStartupParametersKeys::debuggingAuthorized, inAutorized);
}


bool VRIAServerStartupParameters::GetDebuggingAuthorized( bool& outAuthorized) const
{
	return fBag.GetBool( ServerStartupParametersKeys::debuggingAuthorized, outAuthorized);
}


void VRIAServerStartupParameters::SetDebuggerType( WAKDebuggerType_t inType)
{
	fBag.SetLong( ServerStartupParametersKeys::debuggerType, inType);
}


bool VRIAServerStartupParameters::GetDebuggerType( WAKDebuggerType_t &outType) const
{
	return fBag.GetLong( ServerStartupParametersKeys::debuggerType, outType);
}


void VRIAServerStartupParameters::SetAllowDebugAdministrator( bool inAllowDebugAdministrator)
{
	ServerStartupParametersKeys::allowDebugAdministrator.Set( &fBag, inAllowDebugAdministrator);
}


bool VRIAServerStartupParameters::GetAllowDebugAdministrator() const
{
	return ServerStartupParametersKeys::allowDebugAdministrator.Get( &fBag);
}


void VRIAServerStartupParameters::SetAdminLogin( const VString& inLogin)
{
	fBag.SetString( ServerStartupParametersKeys::adminLogin, inLogin);
}


bool VRIAServerStartupParameters::GetAdminLogin( VString& outLogin) const
{
	return fBag.GetString( ServerStartupParametersKeys::adminLogin, outLogin);
}


void VRIAServerStartupParameters::SetAdminPassword( const VString& inPassword)
{
	fBag.SetString( ServerStartupParametersKeys::adminPassword, inPassword);
}


bool VRIAServerStartupParameters::GetAdminPassword( VString& outPassword) const
{
	return fBag.GetString( ServerStartupParametersKeys::adminPassword, outPassword);
}


void VRIAServerStartupParameters::SetJobID( const XBOX::VString& inJobID)
{
	fBag.SetString( ServerStartupParametersKeys::jobID, inJobID);
}


bool VRIAServerStartupParameters::GetJobID( XBOX::VString& outJobID) const
{
	return fBag.GetString( ServerStartupParametersKeys::jobID, outJobID);
}


#if  WITH_SANDBOXED_PROJECT

void VRIAServerStartupParameters::SetWithSandboxedProject( bool inWithSandboxedProject)
{
	ServerStartupParametersKeys::withSandboxedProject.Set( &fBag, inWithSandboxedProject);
}


bool VRIAServerStartupParameters::GetWithSandBoxedProject() const
{
	return ServerStartupParametersKeys::withSandboxedProject.Get( &fBag);
}


void VRIAServerStartupParameters::SetConfigurationFilePath( const XBOX::VFilePath& inPath)
{
	fConfigurationFilePath = inPath;
}


void VRIAServerStartupParameters::GetConfigurationFilePath( XBOX::VFilePath& outPath) const
{
	outPath = fConfigurationFilePath;
}


void VRIAServerStartupParameters::AddProjectToOpen( const VFilePath& inProjectFilePath)
{
	fProjectToOpen.push_back( inProjectFilePath);
}


void VRIAServerStartupParameters::GetProjectsToOpen( std::vector<XBOX::VFilePath>& outProjectFilePaths) const
{
	outProjectFilePaths.clear();
	outProjectFilePaths.insert( outProjectFilePaths.begin(), fProjectToOpen.begin(), fProjectToOpen.end());
}


bool VRIAServerStartupParameters::HasProjectToOpen() const
{
	return !fProjectToOpen.empty();
}

#endif



// ----------------------------------------------------------------------------



VRIAServerStartupMessage::VRIAServerStartupMessage( VRIAServerApplication* inServer, VRIAServerStartupParameters *inStartupParameters)
: fServer(inServer) 
{
	fStartupParameters = RetainRefCountable( inStartupParameters);
}


VRIAServerStartupMessage::~VRIAServerStartupMessage()
{
	ReleaseRefCountable( &fStartupParameters);
}


void VRIAServerStartupMessage::DoExecute()
{
	if (fServer != NULL && (VProjectItemManager::Get() != NULL))
	{
		fServer->_OnStartup( fStartupParameters);
	}
}



// ----------------------------------------------------------------------------



VRIAOpenSolutionAsCurrentSolutionMessage::VRIAOpenSolutionAsCurrentSolutionMessage( VRIAServerApplication* inServer, VSolutionStartupParameters *inStartupParameters, VRIAServerJob *inJob)
: fServer(inServer) 
{
	fStartupParameters = RetainRefCountable( inStartupParameters);
	fJob = RetainRefCountable( inJob);
}


VRIAOpenSolutionAsCurrentSolutionMessage::~VRIAOpenSolutionAsCurrentSolutionMessage()
{
	QuickReleaseRefCountable( fStartupParameters);
	ReleaseRefCountable( &fJob);
}


void VRIAOpenSolutionAsCurrentSolutionMessage::DoExecute()
{
	if (fServer != NULL)
	{
		fServer->_OpenSolutionAsCurrentSolution( fStartupParameters, fJob);
	}
}



// ----------------------------------------------------------------------------



class VRIACloseCurrentSolutionMessage : public VMessage
{
public:
			VRIACloseCurrentSolutionMessage( VRIAServerApplication* inServer) : fServer(inServer) {;}
	virtual	~VRIACloseCurrentSolutionMessage() {;}

protected:
	virtual	void DoExecute()
			{
				if (fServer != NULL)
					fServer->_CloseCurrentSolution();
			}

private:
			VRIAServerApplication	*fServer;
};



// ----------------------------------------------------------------------------



class VDataCacheFlushMessage : public VMessage
{
public:
			VDataCacheFlushMessage( VRIAServerApplication* inServer) : fServer(inServer) {;}
	virtual	~VDataCacheFlushMessage() {;}

protected:
	virtual	void DoExecute()
			{
				if (fServer != NULL)
					fServer->_FlushDataCache();
			}

private:
			VRIAServerApplication	*fServer;
};



// ----------------------------------------------------------------------------







// ----------------------------------------------------------------------------


VRIAServerApplication* VRIAServerApplication::sCurrentApplication = NULL;


VRIAServerApplication::VRIAServerApplication()
: fInitCalled(false)
, fInitOK(false)
, fSolution(NULL)
, fIsOpeningOrClosingCurrentSolution(0)
, fPreventMessageExecutionsAtIdleTime(0)
, fLocalizer(NULL)
, fComponent_DB4D(NULL)
, fComponent_LanguageSyntax(NULL)
, fComponent_HTTP(NULL)
, fComponent_Bridge( NULL)
, fDataCacheFlushEnabled(false)
, fDataCacheFlushDelay(kDEFAULT_DATA_CACHE_FLUSH_DELAY)
, fServiceDiscoveryServer(NULL)
, fStartupParameters(NULL)
, fJSContextPool(NULL)
, fJSRuntimeDelegate(NULL)
, fJSContextMgr(NULL)
, fIndexProgressIndicator(NULL)
, fFlushProgressIndicator(NULL)
{
	xbox_assert(sCurrentApplication == NULL);
	sCurrentApplication = this;
}


VRIAServerApplication::~VRIAServerApplication()
{
	if (fInitCalled)
	{
		_DeInit();
	}
	
	xbox_assert(sCurrentApplication == this);
	sCurrentApplication = NULL;
}

bool VRIAServerApplication::IsEnterpriseVersion()
{
	VString	pdtName;
	VProcess::GetProductName(pdtName);
	return (pdtName.Find("Enterprise",1) > 0);
}

bool VRIAServerApplication::Init( VProcess::InitOptions inOptions)
{
	bool ok = false;
	
	if (!fInitCalled)
	{
		if (inherited::Init( inOptions))
		{
			fInitCalled = true;
			fInitOK = _Init();
		}
	}
	return fInitOK;
}


XBOX::VFolder* VRIAServerApplication::RetainFolder( XBOX::VProcess::EFolderKind inSelector) const
{
	VFolder *folder = NULL;

	switch( inSelector)
	{
		case eFS_Walib:
			{
				VFilePath path;
				GetWALibraryFolderPath( path);
				folder = new VFolder( path);
				break;
			}

		default:
			folder = inherited::RetainFolder( inSelector);
	}

	return folder;
}


VFolder* VRIAServerApplication::RetainApplicationPackageFolder() const
{
	return RetainFolder( VProcess::eFS_Bundle);
}


VFolder* VRIAServerApplication::RetainApplicationResourcesFolder() const
{
	return RetainFolder( VProcess::eFS_Resources);
}


VFolder* VRIAServerApplication::RetainApplicationDTDsFolder() const
{
	return RetainFolder( VProcess::eFS_DTDs);
}


VFolder* VRIAServerApplication::RetainExecutableFolder() const
{
	return RetainFolder( VProcess::eFS_Executable);
}


VFolder* VRIAServerApplication::RetainNativeComponentsFolder() const
{
	return RetainFolder( VProcess::eFS_NativeComponents);
}


void VRIAServerApplication::GetWALibraryFolderPath( VFilePath& outPath) const
{
	// "walib" folder is at the same level than "resources" folder
	VFolder *folder = RetainApplicationResourcesFolder();
	if (folder != NULL)
	{
		VFilePath folderPath;
		folder->GetPath( outPath);
		outPath = outPath.ToParent();
		outPath = outPath.ToSubFolder( L"walib");
		folder->Release();
	}
}


void VRIAServerApplication::GetWAFrameworkFolderPath( VFilePath& outPath) const
{
	GetWALibraryFolderPath( outPath);
	outPath = outPath.ToSubFolder( L"WAF");
}


VFolder* VRIAServerApplication::RetainUserPreferencesFolder( bool inCreateIfNotExists) const
{
	VFolder *folder = NULL;
	
	VFolder *parent = VFolder::RetainSystemFolder( eFK_UserPreferences, inCreateIfNotExists);
	if (parent != NULL)
	{
		VFilePath path;
		parent->GetPath( path);
		path.ToSubFolder( L"WakandaSoftware");
		path.ToSubFolder( L"Server");
		folder = new VFolder( path);
		if (folder != NULL)
		{
			if (!folder->Exists() && inCreateIfNotExists)
				folder->CreateRecursive();
		}
		parent->Release();
	}
	return folder;
}


VFolder* VRIAServerApplication::RetainSolutionsLinkFilesFolder( bool inCreateIfNotExists) const
{
	VFolder *folder = NULL;
	
	VFolder *parent = RetainUserPreferencesFolder( inCreateIfNotExists);
	if (parent != NULL)
	{
		folder = new VFolder( *parent, L"RecentlyOpened");
		if (folder != NULL)
		{
			if (!folder->Exists() && inCreateIfNotExists)
				folder->Create();
		}
		parent->Release();
	}
	return folder;
}


CDB4DManager* VRIAServerApplication::GetComponentDB4D() const
{
	return fComponent_DB4D;
}


CLanguageSyntaxComponent* VRIAServerApplication::GetComponentLanguageSyntax() const
{
	return fComponent_LanguageSyntax;
}


CHTTPServer* VRIAServerApplication::GetComponentHTTP() const
{
	return fComponent_HTTP;
}


VString VRIAServerApplication::GetLogSourceIdentifier() const
{
	return kServerLogSourceIdentifier;
}


VLocalizationManager* VRIAServerApplication::RetainLocalizer() const
{
	VLocalizationManager *localizer = NULL;
	if (fMutex.Lock())
	{
		localizer = RetainRefCountable( fLocalizer);
		fMutex.Unlock();
	}
	return localizer;
}


VSolutionStartupParameters* VRIAServerApplication::RetainDefaultSolutionStartupParameters() const
{
	VSolutionStartupParameters *startupParams = NULL;

	VFolder *folder = RetainApplicationResourcesFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath( path);
		path.ToSubFolder( L"Default Solution");
		path.SetFileName( L"DefaultSolution", false);
		path.SetExtension( RIAFileKind::kSolutionFileExtension);

		VFile *file = new VFile( path);
		if (file != NULL && file->Exists())
		{
			startupParams = new VSolutionStartupParameters();
			if (startupParams != NULL)
			{
				startupParams->GetExtraData().SetBool( L"isDefaultSolution", true);
				startupParams->SetStoreInLinkFile( false);
				startupParams->SetSolutionFileToOpen( file);
				startupParams->SetOpenProjectSymbolsTable( false);	// sc 25/05/2012, on Server, do not use the symbols table anymore

				VRIAServerSolutionOpeningParameters openingParams( startupParams);
				openingParams.SetOpeningMode( eSOM_FOR_RUNNING);
				openingParams.SetOpenDefaultSolutionWhenOpeningFails( false);
				openingParams.SetCustomAdministratorAuthType( L"basic");	// sc 15/06/2012 force basic authentication for admin
				if (fStartupParameters != NULL)
					openingParams.SetAdministratorHandlesDebugger( fStartupParameters->GetAllowDebugAdministrator());
				openingParams.SetCreateAdminUser( true);
				openingParams.UpdateStartupParameters( startupParams);
			}
		}
		QuickReleaseRefCountable( file);
		folder->Release();
	}

	return startupParams;
}


VRIAServerStartupParameters* VRIAServerApplication::RetainStartupParameters() const
{
	return RetainRefCountable( fStartupParameters);
}


XBOX::VError VRIAServerApplication::OpenSolutionAsCurrentSolution( const XBOX::VFilePath& inDesignSolutionFilePath, VRIAServerJob *inJob)
{
	VError err = VE_OK;
	
	VSolutionStartupParameters *startupParams = new VSolutionStartupParameters();

	if (startupParams != NULL)
	{
		VFile *file = new VFile( inDesignSolutionFilePath);
		startupParams->SetSolutionFileToOpen( file);
		startupParams->SetOpenProjectSymbolsTable( false);	// sc 25/05/2012, on Server, do not use the symbols table anymore
		QuickReleaseRefCountable( file);
	}

	err = OpenSolutionAsCurrentSolution( startupParams, inJob);

	QuickReleaseRefCountable( startupParams);

	return err;
}


XBOX::VError VRIAServerApplication::OpenSolutionAsCurrentSolution( VSolutionStartupParameters *inStartupParameters, VRIAServerJob *inJob)
{
	VError err = VE_OK;

	VRIAOpenSolutionAsCurrentSolutionMessage *msg = new VRIAOpenSolutionAsCurrentSolutionMessage( this, inStartupParameters, inJob);
	if (msg != NULL)
	{
		fMessageQueue.AddMessage( msg);
		msg->Release();
	}

	return err;
}


VError VRIAServerApplication::CloseCurrentSolution()
{
	VError err = VE_OK;

	VRIACloseCurrentSolutionMessage *msg = new VRIACloseCurrentSolutionMessage( this);
	if (msg != NULL)
	{
		fMessageQueue.AddMessage( msg);
		msg->Release();
	}
	return err;
}


VRIAServerSolution* VRIAServerApplication::RetainCurrentSolution() const
{
	VRIAServerSolution *solution = NULL;
	if (fSolutionMutex.Lock())
	{
		solution = RetainRefCountable( fSolution);
		fSolutionMutex.Unlock();
	}
	return solution;
}


bool VRIAServerApplication::IsCurrentSolution( VRIAServerSolution *inSolution) const
{
	bool result = false;

	if (fSolutionMutex.Lock())
	{
		result = (inSolution == fSolution);
		fSolutionMutex.Unlock();
	}

	return result;
}


VRIAServerSolution* VRIAServerApplication::OpenAndRetainSolutionForMaintenance( XBOX::VError& outError,  VSolutionStartupParameters *inStartupParameters, VRIAServerJob *inJob)
{
	outError = VE_OK;

	VRIAServerSolution *solution = NULL;
	VValueBag *jobOpenBag = NULL;

	VRIAServerSolutionOpeningParameters openingParams( inStartupParameters);
	assert(openingParams.GetOpeningMode() == eSOM_FOR_MAINTENANCE);

	if (inJob != NULL)
	{
		jobOpenBag = new VValueBag;
		if (jobOpenBag != NULL)
			JobCommonBagKeys::openingMode.Set( jobOpenBag, L"maintenance");
	}

	if (inStartupParameters != NULL)
	{
		VFile *solutionFile = inStartupParameters->GetSolutionFileToOpen();
		if (solutionFile != NULL)
		{
			LogMessage( L"", eL4JML_Information, L"Opens the solution \"" + solutionFile->GetPath().GetPath() + L"\" for maintenance");
		}
	}

	VMicrosecondsCounter usCounter;
	usCounter.Start();

	solution = VRIAServerSolution::OpenSolution( outError, inStartupParameters);

	if (solution != NULL)
	{
		VString logMsg;
		logMsg.Printf( "\"%S\" solution has been successfully opened for maintenance (duration: %i ms)", &solution->GetName(), usCounter.Stop()/1000);
		LogMessage( L"", eL4JML_Information, logMsg);
	
		if (jobOpenBag != NULL) 
			JobCommonBagKeys::solutionName.Set( jobOpenBag, solution->GetName());

	}
	else if (inStartupParameters != NULL)
	{
		VFile *solutionFile = inStartupParameters->GetSolutionFileToOpen();
		if (solutionFile != NULL)
			outError = vThrowError( VE_RIA_CANNOT_OPEN_SOLUTION_FOR_MAINTENANCE, solutionFile->GetPath().GetPath());
	}

	if (inJob != NULL)
	{
		if (jobOpenBag != NULL)
		{
			JobCommonBagKeys::jobResult.Set( jobOpenBag, (outError == VE_OK) ? 0 : 1);
			ILoggerBagKeys::message.Set( jobOpenBag, (outError == VE_OK) ? L"solution opened" : L"cannot open the solution");
		}
		VRIAServerSupervisor::Get()->TerminateJob( inJob->GetId(), jobOpenBag);
	}
	ReleaseRefCountable( &jobOpenBag);

	return solution;
}


VError VRIAServerApplication::PostMessage( VMessage *inMessage)
{
	fMessageQueue.AddMessage( inMessage);
	return VE_OK;
}


void VRIAServerApplication::SetDataCacheFlushEnabled( bool inEnabled)
{
	if (fDataCacheMutex.Lock())
	{
		if (inEnabled != fDataCacheFlushEnabled)
		{
			fDataCacheFlushEnabled = inEnabled;
			if (fDataCacheFlushEnabled)
			{
				if (fDataCacheFlushDelay < kMIN_DATA_CACHE_FLUSH_DELAY)
					fDataCacheFlushDelay = kMIN_DATA_CACHE_FLUSH_DELAY;
				
				VDataCacheFlushMessage *msg = new VDataCacheFlushMessage( this);
				if (msg != NULL)
				{
					msg->PostTo( VTaskMgr::Get()->GetMainTask(), false, fDataCacheFlushDelay);
					msg->Release();
				}
			}
		}
		fDataCacheMutex.Unlock();
	}
}


bool VRIAServerApplication::IsDataCacheFlushEnabled() const
{
	bool enabled = false;
	if (fDataCacheMutex.Lock())
	{
		enabled = fDataCacheFlushEnabled;
		fDataCacheMutex.Unlock();
	}
	return enabled;
}


void VRIAServerApplication::SetDataCacheFlushDelay( sLONG inDelay)
{
	if (fDataCacheMutex.Lock())
	{
		fDataCacheFlushDelay = (inDelay < kMIN_DATA_CACHE_FLUSH_DELAY) ? kMIN_DATA_CACHE_FLUSH_DELAY : inDelay;
		fDataCacheMutex.Unlock();
	}
}


sLONG VRIAServerApplication::GetDataCacheFlushDelay() const
{
	sLONG delay = 0;
	if (fDataCacheMutex.Lock())
	{
		delay = fDataCacheFlushDelay;
		fDataCacheMutex.Unlock();
	}
	return delay;
}


VRIAServerJSContextMgr* VRIAServerApplication::GetJSContextMgr() const
{
	return fJSContextMgr;
}


XBOX::VJSGlobalContext* VRIAServerApplication::RetainJSContext( XBOX::VError& outError, bool inReusable)
{
	return (fJSContextPool != NULL) ? fJSContextPool->RetainContext( outError, inReusable) : NULL;
}


XBOX::VError VRIAServerApplication::ReleaseJSContext( XBOX::VJSGlobalContext* inContext)
{
	VError err = VE_OK;
	
	if (fJSContextPool != NULL)
		err = fJSContextPool->ReleaseContext( inContext);

	return err;
}


#if WITH_SANDBOXED_PROJECT

bool VRIAServerApplication::GetHandlesSandboxedProject() const
{
	if (fStartupParameters != NULL)
		return fStartupParameters->GetWithSandBoxedProject();

	return false;
}


VRIAServerProject* VRIAServerApplication::OpenAndRetainProject( VError& outError, const VFile& inProjectFile, VRIAServerProjectOpeningParameters* inProjectParams)
{
	VRIAServerProject *project = NULL;
	outError = VE_OK;

	if (inProjectFile.Exists())
	{
		VFilePath projectFilePath;
		inProjectFile.GetPath( projectFilePath);

		VProject *designProject = VProject::Instantiate( outError, NULL, projectFilePath);
		if ((designProject != NULL) && (outError == VE_OK))
		{
			outError = designProject->Load( false);
			if (outError == VE_OK)
			{
				project = VRIAServerProject::OpenProject( outError, NULL, designProject, inProjectParams);
				if (project != NULL)
				{
					if (fProjectsCollectionMutex.Lock())
					{
						fProjectsCollection.push_back( VRefPtr<VRIAServerProject>(project));
						fProjectsCollectionMutex.Unlock();
					}
				}
			}
		}
		ReleaseRefCountable( &designProject);
	}
	else
	{
		outError = vThrowError( VE_FILE_NOT_FOUND);
	}

	return project;
}


VError VRIAServerApplication::CloseProject( VRIAServerProject* inProject)
{
	if (inProject == NULL)
		return VE_INVALID_PARAMETER;

	VError err = VE_OK;

	if (inProject->IsStarted())
		StopProject( inProject);

	VProject *designProject = RetainRefCountable( inProject->GetDesignProject());
	
	err = inProject->Close();
	if (fProjectsCollectionMutex.Lock())
	{
		VectorOfApplication_citer found = std::find( fProjectsCollection.begin(), fProjectsCollection.end(), VRefPtr<VRIAServerProject>(inProject));
		if (found != fProjectsCollection.end())
			fProjectsCollection.erase( found);
		fProjectsCollectionMutex.Unlock();
	}
	
	if (testAssert( designProject != NULL))
	{
		designProject->Unload();
		designProject->Release();
	}

	return err;
}


VError VRIAServerApplication::StartProject( VRIAServerProject* inProject)
{
	if (inProject == NULL)
		return VE_INVALID_PARAMETER;

	VError err =  inProject->Start();
	if (err == VE_OK)
		err = inProject->OnStartup();
	return err;
}


VError VRIAServerApplication::StopProject( VRIAServerProject* inProject)
{
	if (inProject == NULL)
		return VE_INVALID_PARAMETER;

	VError err =  inProject->OnStop();
	if (err == VE_OK)
		err = inProject->Stop();
	return err;
}


VRIAServerProject* VRIAServerApplication::RetainProjectByName( const VString& inName) const
{
	VRIAServerProject *project = NULL;
	if (fProjectsCollectionMutex.Lock())
	{
		VString locName;
		for (VectorOfApplication_citer iter = fProjectsCollection.begin() ; (iter != fProjectsCollection.end()) && (project == NULL) ; ++iter)
		{
			(*iter)->GetName( locName);
			if (inName.EqualToString( locName, true))
				project = (*iter).Retain();
		}
		fProjectsCollectionMutex.Unlock();
	}
	return project;
}


void VRIAServerApplication::RetainAllProjects( VectorOfApplication& outProjects) const
{
	if (fProjectsCollectionMutex.Lock())
	{
		outProjects.clear();
		outProjects.insert( outProjects.begin(), fProjectsCollection.begin(), fProjectsCollection.end()),

		fProjectsCollectionMutex.Unlock();
	}
}

#endif


void VRIAServerApplication::DoRun()
{
	// sc 19/02/2013 WAK0080519, keep only the last fifteen valid link files
	TimeToStringsPairMultimap linkFiles;
	GetMapOfRecentSolutionFiles( linkFiles);

	sLONG linkFileCount = 0;
	TimeToStringsPairMultimap::reverse_iterator linkIter = linkFiles.rbegin();
	while (linkIter != linkFiles.rend())
	{
		VFile linkFile( linkIter->second.second);
			
		bool validLinkFile = linkFileCount < 15;
		if (validLinkFile)
		{
			VSolutionStartupParameters startupParams;
			validLinkFile = (LoadSolutionStartupParametersFromLinkFile( linkFile, startupParams) == VE_OK);
			if (validLinkFile)
			{
				VFile *solutionFile = startupParams.GetSolutionFileToOpen();
				if (solutionFile != NULL)
					validLinkFile = solutionFile->Exists();
			}
		}

		if (validLinkFile)
			++linkFileCount;
		else
			linkFile.Delete();

		++linkIter;
	}

	StartIdling( VTask::GetCurrent());

	inherited::DoRun();
}


void VRIAServerApplication::DoQuit()
{
	// Close the current solution
	VError err = _CloseCurrentSolution();

#if WITH_SANDBOXED_PROJECT
	if (GetHandlesSandboxedProject())
	{
		_CloseAllProjects();
		VRoutingPreProcessingHandler::DeInit();
	}
#endif // WITH_SANDBOXED_PROJECT

	StopIdling();

	inherited::DoQuit();
}


void VRIAServerApplication::DoIdle()
{
	if (fPreventMessageExecutionsAtIdleTime == 0)
	{
		++fPreventMessageExecutionsAtIdleTime; // reentrance safeguard

		VMessage *msg = fMessageQueue.RetainMessage();
		if (msg != NULL)
		{
			msg->ActivateContext( VTask::GetCurrent());
			msg->Execute();
			msg->Release();
		}

		--fPreventMessageExecutionsAtIdleTime;
	}
}


XBOX::IJSRuntimeDelegate* VRIAServerApplication::GetRuntimeDelegate()
{
	return fJSRuntimeDelegate;
}


XBOX::VError VRIAServerApplication::InitializeJSContext( XBOX::VJSGlobalContext* inContext)
{
	VError err = VE_OK;

	if (inContext != NULL)
	{
		err = VRIAJSRuntimeContext::InitializeJSContext( inContext, NULL);
	}
	return err;
}


XBOX::VError VRIAServerApplication::UninitializeJSContext( XBOX::VJSGlobalContext* inContext)
{
	VError err = VE_OK;
	
	if (inContext != NULL)
	{
		err = VRIAJSRuntimeContext::UninitializeJSContext( inContext);
	}
	return err;
}


bool VRIAServerApplication::_Init()
{
	bool ok = false;
	
	VDebugMgr::Get()->SetMessagesEnabled( true); // sc 03/05/2013, to enable errors logging

    #if !VERSION_LINUX
	// pp init gdiplus
	VGraphicContext::Init();

	VPicture::Init(NULL);
	//Force le lien avec la Dll Graphics
	XBOX::VPicture* tmp=new XBOX::VPicture();
	delete tmp;
	#endif

	SetMessagingTask( VTask::GetCurrentID());
	
	// Init KernelIPC lib
	ok = (VComponentManager::Init() != 0);
	if (ok)
		ok = VCommand::Init();
	
	// Init XML lib
	if (ok)
		VXMLParser::Init();
	if (ok)
		VUTIManager::Init();

	VFolder *resFolder = RetainApplicationResourcesFolder();

	// Init localization manager
	if (ok)
	{
		ok = (fLocalizer = new VLocalizationManager( DC_ENGLISH_US)) != NULL;
		if (ok)
		{
			ok = fLocalizer->LoadDefaultLocalizationFolders( resFolder);
		}
	}

#if VERSIONWIN || VERSION_LINUX
	// Load BasicUTIs.plist
	if (ok)
	{
		VFile basicUTIsPListFile( *resFolder, CVSTR( "BasicUTIs.plist"));
		if (basicUTIsPListFile.Exists())
		{
			ok = VUTIManager::GetUTIManager()->LoadXMLFile( &basicUTIsPListFile);
		}
	}
	
	// Load Info.plist
	if (ok)
	{
		VFile infoPListFile( *resFolder, CVSTR( "Info.plist"));
		if (infoPListFile.Exists())
		{
			ok = VUTIManager::GetUTIManager()->LoadXMLFile( &infoPListFile);
		}
	}
#endif

	// Init errors manager
	if (ok)
	{
		VErrorBase::RegisterLocalizer( kXTOOLBOX_SIGNATURE, fLocalizer);
		VErrorBase::RegisterLocalizer( kSERVER_NET_SIGNATURE, fLocalizer);
		VErrorBase::RegisterLocalizer( kCOMPONENT_XML, fLocalizer);
		VErrorBase::RegisterLocalizer( kJAVASCRIPT_SIGNATURE, fLocalizer);
		VErrorBase::RegisterLocalizer( kRIA_OSTYPE_SIGNATURE, fLocalizer);
		VErrorBase::RegisterLocalizer( kXHR_SIGNATURE, fLocalizer);
		VErrorBase::RegisterLocalizer( 'CWRP' /*Curl Wrapper, used in XHR impl.*/ , fLocalizer);
	}
	
	// install default filesystems
	if (ok)
		ok = _InitFileSystems();

	// Load all built-in components here. Built-in components may be used by the ones in external dynamic libraries
	if (ok)
	{
		sLONG kCOMPONENT_TYPE_COUNT_BRIDGE = 1;
		CImpDescriptor kCOMPONENT_TYPE_LIST_BRIDGE [ ] = { { CRIAServerComponentBridge::Component_Type, VImpCreator<VRIAServerComponentBridge>::CreateImp } };
		new VComponentLibrary( kCOMPONENT_TYPE_LIST_BRIDGE, kCOMPONENT_TYPE_COUNT_BRIDGE);
		VComponentManager::RegisterComponentCreator( CRIAServerComponentBridge::Component_Type, VRIAServerComponentBridge::ComponentCreator, true);
		fComponent_Bridge = VComponentManager::RetainComponent<CRIAServerComponentBridge>();
		ok = (fComponent_Bridge != NULL);
	}

	// Init components manager
	if (ok)
	{
		VFolder* componentsFolder = RetainNativeComponentsFolder();
	
		if (testAssert( componentsFolder != NULL))
		{
			for (VFolderIterator i( componentsFolder) ; i.IsValid() ; ++i) 
			{
				VString	libName;
				i->GetName( libName);
			#if VERSIONDEBUG
				// register only needed components (the folder contains others 4D products components)
				if (	libName == CVSTR("DB4DDebug.bundle")
					||	libName == CVSTR("HttpServerDebug.bundle")
					||	libName == CVSTR("UsersAndGroupsDebug.bundle")
					||	libName == CVSTR("SecurityManagerDebug.bundle")
					||	libName == CVSTR("ZipDebug.bundle")
					||	libName == CVSTR("MySQLConnectorDebug.bundle")
                    			||	libName == CVSTR("MSSQLConnectorDebug.bundle")
                    		      ||	libName == CVSTR("ODBCConnectorDebug.bundle")
					||	libName == CVSTR("LanguageSyntaxDebug.bundle")
					||	libName == CVSTR("SQLModelDebug.bundle"))
			#endif
				{
					VError error = VComponentManager::RegisterComponentLibrary( i->GetPath());
					if (error == VE_OK)
						DebugMsg( "Loaded %V\n", &libName);
				}
			}
		}
		ReleaseRefCountable( &componentsFolder);
	}

	// Init DB4D component
	if (ok)
	{
		ok = (fComponent_DB4D = CDB4DManager::RetainManager(fLocalizer)) != NULL;
		if (fComponent_DB4D != NULL)
		{
			fIndexProgressIndicator = new VRIAServerProgressIndicator(RIASERVER_PROGRESS_INDEX_USERINFO, GetPublishEventSignal() );
			fFlushProgressIndicator = new VRIAServerProgressIndicator(RIASERVER_PROGRESS_FLUSH_USERINFO, GetPublishEventSignal() );
			GetPublishEventSignal()->Connect(this, XBOX::VTask::GetCurrent(), &VRIAServerApplication::PublishServiceRecordEvent);
			
			fComponent_DB4D->SetRunningMode( DB4D_RunningWakanda);	// important so that db4D uses proper file extension
			fComponent_DB4D->SetDefaultProgressIndicator_For_Indexes(fIndexProgressIndicator);
			fComponent_DB4D->SetDefaultProgressIndicator_For_DataCacheFlushing(fFlushProgressIndicator);
		}
	}
	
	// Init Language Syntax Component
	if (ok)
		ok = (fComponent_LanguageSyntax = VComponentManager::RetainComponentOfType< CLanguageSyntaxComponent >()) != NULL;

	// Init HTTP Server Component
	if (ok)
		ok = (fComponent_HTTP = VComponentManager::RetainComponent< CHTTPServer >()) != NULL;
	
	if (ok)
		VTaskMgr::Get()->SetNeedsCheckSystemMessages( true);
		
	// Init ServerNet
	if (ok)
		VServerNetManager::Init(NULL /*ICriticalError*/, IpAnyIsV6 /*IP V4 or V6 policy*/);
	
	// Init design solution manager
	if (ok)
		VSolutionManager::Init( false);	// sc 08/02/2013 WAK0080397, the unique ID of project items is not used on Wakanda Server

	// Init service discovery.
	if (ok) {
		
		if ((fServiceDiscoveryServer = VServiceDiscoveryServer::GetInstance()) == NULL) 
			
			ok = false;

		else

			fServiceDiscoveryServer->Start();
	}
	
	if (ok)
	{
		fJSContextMgr = new VRIAServerJSContextMgr();
		ok = (fJSContextMgr != NULL);
	}

	if (ok)
	{
		VJSWorker::SetDelegate( fJSContextMgr);
	}

	if (fComponent_Bridge != NULL)
	{
		VRIAServerComponentBridge *bridge = XBOX::VImpCreator<VRIAServerComponentBridge>::GetImpObject(fComponent_Bridge);
		if ( fComponent_DB4D != NULL )
		{
			fComponent_DB4D->SetGraphicsInterface( bridge);
			fComponent_DB4D->SetApplicationInterface( bridge);
		}
	}


	if (ok)
		ok = VJSContextPool::Init();	// sc 20/04/2011, initialize the global object class here in case of a JavaScript context is created outside the VJSContextPool.

	if (ok)
		ok = VRIAServerSupervisor::Init();

	QuickReleaseRefCountable( resFolder);
	
#if VERSIONMAC && USE_HELPER_TOOLS
	if (ok)
	{
		AuthorizationHelpers::Init();

		// Helper Tools is not installed. Let's try to do it...
		if (!AuthorizationHelpers::IsHelperToolInstalled())
			AuthorizationHelpers::InstallHelperTool();
	}
#endif

#if VERSIONDEBUG
	StTaskPropertiesSetter::_Test();
#endif

	return ok;
}


void VRIAServerApplication::_DeInit()
{
	xbox_assert(fSolution == NULL);

	VRIAServerSupervisor::DeInit();
	
	VJSWorker::SetDelegate( NULL);

	delete fJSContextMgr;
	fJSContextMgr = NULL;

	if (fServiceDiscoveryServer != NULL) {
		
		fServiceDiscoveryServer->KillAndWaitTermination();
		fServiceDiscoveryServer->Release();

		fServiceDiscoveryServer = NULL;	
		
	}
 
	ReleaseRefCountable( &fStartupParameters);

	// DeInit the component bridge
	if (fComponent_Bridge != NULL)
	{
		fComponent_Bridge->StopSharedWorkerPool();
		fComponent_Bridge->StopSharedSelectIOPool();
	}
	
	// Stop the language syntax component
	if (fComponent_LanguageSyntax != NULL)
		fComponent_LanguageSyntax->Stop();
	
	// DeInit design solution manager
	VSolutionManager::DeInit();

	VServerNetManager::DeInit();
	
	// DeInit native components
	ReleaseRefCountable( &fComponent_DB4D);
	ReleaseRefCountable( &fComponent_LanguageSyntax);
	ReleaseRefCountable( &fComponent_HTTP);
	ReleaseRefCountable( &fComponent_Bridge);	// must be released after db4d that uses it

	// DeInit errors manager
	VErrorBase::UnregisterLocalizer( kRIA_OSTYPE_SIGNATURE);
	
	// DeInit localization manager
	ReleaseRefCountable( &fLocalizer);

	// Deinit XML lib
	VUTIManager::DeInit();
	VXMLParser::DeInit();
	
	// Deinit KernelIPC lib
	VCommand::DeInit();
	VComponentManager::DeInit();
	
    #if !VERSION_LINUX
	//JQ 13/12/2010: fixed crash
	VGraphicContext::DeInit();
	#endif

	// Finally, kill remaining tasks
	sLONG taskCount = 0;
	
	VTaskMgr::Get()->KillAllTasks( 3000);
	
	std::vector<VString> taskNames;
	std::vector< VRefPtr<VTask> > tasks;
	VTaskMgr::Get()->GetTasks( tasks);
	for (std::vector< VRefPtr<VTask> >::iterator i = tasks.begin() ; i != tasks.end() ; ++i)
	{
		if ( ((*i)->GetState() != TS_DEAD) && !(*i)->IsMain())
		{
			VString taskName;
			(*i)->GetName( taskName);
			taskNames.push_back( taskName);
			++taskCount;
		}
	}
	
	if (taskCount > 0)
	{
		for (std::vector<VString>::iterator iter = taskNames.begin() ; iter != taskNames.end() ; ++iter)
		{
			VString msg;
			msg.FromCString( "Task \"");
			msg.AppendString( *iter);
			msg.AppendCString( "\" is still running.\n");
			DebugMsg( msg);
		}
	}

	GetPublishEventSignal()->Disconnect(this);
	
	ReleaseRefCountable( &fIndexProgressIndicator);
	ReleaseRefCountable( &fFlushProgressIndicator);

#if VERSIONMAC && USE_HELPER_TOOLS
	AuthorizationHelpers::DeInit();
#endif
}


bool VRIAServerApplication::_InitFileSystems()
{
	//walib
	VFilePath path( VProcess::Get()->GetExecutableFolderPath());

#if VERSIONMAC
	path.ToParent();
#endif

	path.ToSubFolder( CVSTR( "walib"));
	GetFileSystemNamespace()->RegisterFileSystem( CVSTR( "WALIB"), path, eFSO_Default);

	VFolder *folder = RetainFolder( eFS_Resources);
	path = folder->GetPath();
	path.ToSubFolder( CVSTR ("Web Components"));
	GetFileSystemNamespace()->RegisterFileSystem( CVSTR( "WEB_COMPONENTS"), path, eFSO_Default);
	ReleaseRefCountable( &folder);
	
	return true;
}


void VRIAServerApplication::_OnStartup( VRIAServerStartupParameters *inStartupParameters)
{
	bool continueRunning = false;

	_CleanupUserCacheFolder();

	if (testAssert(fStartupParameters == NULL))
	{
		CopyRefCountable( &fStartupParameters, inStartupParameters);
		if (fStartupParameters != NULL)
		{
			VSolutionStartupParameters *solutionStartupParameters = NULL;
			
			VFile *solutionFile = fStartupParameters->GetSolutionToLaunch();
			if (solutionFile != NULL)
			{
				solutionStartupParameters = new VSolutionStartupParameters();
				if (solutionStartupParameters != NULL)
				{
					VRIAServerSolutionOpeningParameters openingParams( solutionStartupParameters);
					openingParams.SetOpeningMode( eSOM_FOR_RUNNING);
					openingParams.UpdateStartupParameters( solutionStartupParameters);
					solutionStartupParameters->SetSolutionFileToOpen( solutionFile);
					solutionStartupParameters->SetOpenProjectSymbolsTable( false);	// sc 25/05/2012, on Server, do not use the symbols table anymore
				}
			}
			else
			{
				VFile *jsFile = fStartupParameters->GetJavaScriptFileToExecute();
				if (jsFile != NULL)
				{
					VError err = VE_OK;

					assert(fJSContextPool == NULL && fJSRuntimeDelegate == NULL);

					VFolder *parentFolder = jsFile->RetainParentFolder();
					
					fJSRuntimeDelegate = new VRIAServerJSRuntimeDelegate( parentFolder);
					if (fJSRuntimeDelegate != NULL)
					{
						fJSContextPool = fJSContextMgr->CreateJSContextPool( err, this);
						if (fJSContextPool == NULL)
							err = vThrowError( VE_MEMORY_FULL);

						if (err == VE_OK)
						{
							VJSGlobalContext *globalContext = RetainJSContext( err, false);
							if (globalContext != NULL)
							{
								if (err == VE_OK)
								{
									VValueSingle *result = NULL;
									StErrorContextInstaller errorContext;

									globalContext->EvaluateScript( jsFile, &result, true);

									if (errorContext.GetLastError() != VE_OK)
									{
										VErrorContext *context = errorContext.GetContext();
										if (context != NULL)
										{
											VString errorMessage;
											for (std::vector<VRefPtr<VErrorBase> >::const_iterator iter = context->GetErrorStack().begin() ; iter != context->GetErrorStack().end() ; ++iter)
											{
												(*iter)->GetErrorDescription( errorMessage);
												errorMessage.AppendUniChar( L'\n');
												StStringConverter<char> stringConverter( VTC_StdLib_char);
												fputs( stringConverter.ConvertString( errorMessage), stdout);
											}
										}
									}
									else if (result != NULL)
									{
										VString stringResult;
										result->GetString( stringResult);
										stringResult.AppendUniChar( L'\n');
										StStringConverter<char> stringConverter( VTC_StdLib_char);
										fputs( stringConverter.ConvertString( stringResult), stdout);
									}
								}

								err = ReleaseJSContext( globalContext);
								globalContext = NULL;
							}

							// sc 22/03/2013, review context pool cleaning mechanism
							if (VRIAServerApplication::Get()->GetDebuggingAuthorized())
								VJSGlobalContext::AbortAllDebug();

							VJSWorker::TerminateAll();

							fJSContextPool->SetEnabled( false);

							if (fJSContextPool != NULL)
							{
								VSyncEvent *syncEvent = fJSContextPool->WaitForNumberOfUsedContextEqualZero();
								if (syncEvent != NULL)
								{
									syncEvent->Lock( 5000);
									syncEvent->Release();
								}
								fJSContextPool->Clean();
							}

							delete fJSContextPool;
							fJSContextPool = NULL;
						}

						delete fJSRuntimeDelegate;
						fJSRuntimeDelegate = NULL;
					}
					else
					{
						err = vThrowError( VE_MEMORY_FULL);
					}
					
					ReleaseRefCountable( &parentFolder);
				}
				else
				{
				#if WITH_SANDBOXED_PROJECT
					if (GetHandlesSandboxedProject())
					{
						// Load the configuration
						VError err = VE_OK;
						VRIAServerConfiguration *config = NULL;
						VFilePath path;
						
						fConfigurationFolderPath.Clear();

						fStartupParameters->GetConfigurationFilePath( path);
						if (!path.IsEmpty())
						{
							VFile file( path);
							if (file.Exists())
							{
								config = new VRIAServerConfiguration();
								if (config != NULL)
								{
									err = config->Load( file);
									if (err == VE_OK)
										config->GetFolderPath( fConfigurationFolderPath);
								}
								else
								{
									err = vThrowError( VE_MEMORY_FULL);
								}
							}
						}

						if ((err == VE_OK) && (config == NULL))
						{
							VFolder *tempFolder = VProcess::Get()->RetainProductSystemFolder( eFK_UserCache, true);
							if (tempFolder != NULL)
							{
								VString folderName( L"DefaultConfiguration");
								folderName.AppendUniChar( L'-').AppendULong8( VProcess::Get()->GetSystemID());
								
								tempFolder->GetPath( fConfigurationFolderPath);
								fConfigurationFolderPath.ToSubFolder( folderName);

								VFolder config( fConfigurationFolderPath);
								if (!config.Exists())
									config.CreateRecursive( false);
								
								tempFolder->Release();
							}
						}

						if (err == VE_OK)
						{
							err = _OpenServerAdminProject();
							if ((err == VE_OK) && (config != NULL))
							{
								std::vector<VFilePath> projectsToOpen;
								config->GetProjects( projectsToOpen);
								for (std::vector<VFilePath>::iterator iter = projectsToOpen.begin() ; iter != projectsToOpen.end() ; ++iter)
								{
									VRIAServerProjectOpeningParameters *openingParams = new VRIAServerProjectOpeningParameters();
									if (openingParams != NULL)
									{
										openingParams->SetOpeningMode( ePOM_FOR_RUNNING | ePOM_SANDBOXED);

										VError lError = VE_OK;
										VFile projectFile( *iter);
										VRIAServerProject *project = OpenAndRetainProject( lError, projectFile, openingParams);
										if (project != NULL)
										{
											if (lError == VE_OK)
											{
												lError = StartProject( project);
												if (lError != VE_OK)
													StopProject( project);
											}

											if (lError != VE_OK)
												CloseProject( project);

											ReleaseRefCountable( &project);
										}
										openingParams->Release();
									}
								}
							}
						}
						ReleaseRefCountable( &config);

						if ((err == VE_OK) && fStartupParameters->HasProjectToOpen())
						{
							std::vector<VFilePath> projectsToOpen;
							fStartupParameters->GetProjectsToOpen( projectsToOpen);
							for (std::vector<VFilePath>::iterator iter = projectsToOpen.begin() ; iter != projectsToOpen.end() ; ++iter)
							{
								VRIAServerProjectOpeningParameters *openingParams = new VRIAServerProjectOpeningParameters();
								if (openingParams != NULL)
								{
									openingParams->SetOpeningMode( ePOM_FOR_RUNNING | ePOM_SANDBOXED);

									VError lError = VE_OK;
									VFile projectFile( *iter);
									VRIAServerProject *project = OpenAndRetainProject( lError, projectFile, openingParams);
									if (project != NULL)
									{
										if (lError == VE_OK)
										{
											lError = StartProject( project);
											if (lError != VE_OK)
												StopProject( project);
										}

										if (lError != VE_OK)
											CloseProject( project);

										ReleaseRefCountable( &project);
									}
									openingParams->Release();
								}
							}
						}

						continueRunning = (err == VE_OK);
					}
					else
				#endif // WITH_SANDBOXED_PROJECT
					{
						solutionStartupParameters = RetainDefaultSolutionStartupParameters();
					}
				}
			}

			if (solutionStartupParameters != NULL)
			{
				VString jobID;
				VRIAServerJob *job = NULL;
				if (fStartupParameters->GetJobID( jobID))
					job = VRIAServerSupervisor::Get()->RetainJob( jobID);

				VError err = _OpenSolutionAsCurrentSolution( solutionStartupParameters, job);
				continueRunning = (err == VE_OK);	// sc 13/02/2012, quit the server if none solution is opened

				ReleaseRefCountable( &job);
			}

			ReleaseRefCountable( &solutionStartupParameters);
		}
	}

	if (!continueRunning)
	{
		QuitAsynchronously();
	}
	else
	{
		VRIAServerSupervisor::Get()->AcceptClientConnexions();
	}
}


VError VRIAServerApplication::_OpenSolutionAsCurrentSolution( VSolutionStartupParameters *inStartupParameters, VRIAServerJob *inJob)
{
	xbox_assert(VTaskMgr::Get()->GetCurrentTaskID() == VTaskMgr::Get()->GetMainTask()->GetID());

	VValueBag *jobOpenBag = NULL;
	bool terminateJob = true;
	VError err = VE_OK;

	if (inJob != NULL)
	{
		jobOpenBag = new VValueBag;
		if (jobOpenBag != NULL)
		{
			JobCommonBagKeys::openingMode.Set( jobOpenBag, L"running");
			if (inStartupParameters != NULL)
			{
				VRIAServerSolutionOpeningParameters openingParams( inStartupParameters);
				WAKDebuggerType_t dbgrType = NO_DEBUGGER_TYPE;
				if (openingParams.GetDebuggerType( dbgrType))
					JobCommonBagKeys::debuggerParam.Set( jobOpenBag, DebuggerTypeToDebuggerParam( dbgrType));
				bool value = false;
				if (inStartupParameters->GetExtraData().GetBool( L"isDefaultSolution", value))
					JobCommonBagKeys::isDefaultSolution.Set( jobOpenBag, value);
			}
		}
	}

	if (fIsOpeningOrClosingCurrentSolution == 0)	// sc 21/06/2012 reentrance safeguard
	{
		++fIsOpeningOrClosingCurrentSolution;

		VString jobID((inJob != NULL) ? inJob->GetId() : L"");
		StTaskPropertiesSetter taskPropertiesSetter( NULL, (inJob != NULL) ? &jobID : NULL);

		// first, close previous opened solution
		// sc 21/01/2010, to avoid dead locks, never lock the solution mutex during the solution stopping.
		if (fSolution != NULL)
		{
			LogMessage( L"", eL4JML_Information, L"Stops \"" + fSolution->GetName() + L"\" solution");

			VSolution *designSolution = fSolution->GetDesignSolution();
			if (designSolution != NULL)
			{
				designSolution->StopWatchingFileSystem();
				designSolution->StopUpdatingSymbolTable();
			}

			err = fSolution->Stop();
			xbox_assert(err == VE_OK);
		}

		// sc/mc 17/04/2013, the solution mutex does not need to be locked while the new solution is opened
		if (fSolution != NULL)
		{
			LogMessage( L"", eL4JML_Information, L"Closes \"" + fSolution->GetName() + L"\" solution");

			VValueBag *jobCloseBag = NULL;
			if (inJob != NULL)
			{
				jobCloseBag  = new VValueBag;
				if (jobCloseBag != NULL)
					JobCommonBagKeys::solutionName.Set( jobCloseBag, fSolution->GetName());
			}
			_WithdrawServiceRecord(DEFAULT_SERVICE_NAME, fSolution->GetName());
			
			err = fSolution->Close();
			xbox_assert(err == VE_OK);

			if (fSolutionMutex.Lock())
			{
				ReleaseRefCountable( &fSolution);
				fSolutionMutex.Unlock();
			}

			if (inJob != NULL)
			{
				if (jobCloseBag != NULL)
				{
					JobCommonBagKeys::jobResult.Set( jobCloseBag, (err == VE_OK) ? 0 : 1);
					ILoggerBagKeys::message.Set( jobCloseBag, (err == VE_OK) ? L"solution closed" : L"cannot close the solution");
				}
				inJob->Log( jobCloseBag);
			}
			ReleaseRefCountable( &jobCloseBag);
		}

		VRIAServerSolution *solution = NULL;
		
		VRIAServerSolutionOpeningParameters openingParams( inStartupParameters);
		sLONG adminHttpPort = -1, adminSSLPort = -1;
		WAKDebuggerType_t dbgType = NO_DEBUGGER_TYPE;
		
		assert(openingParams.GetOpeningMode() == eSOM_FOR_RUNNING);
		if (fStartupParameters->GetAdministratorHttpPort( adminHttpPort))
			openingParams.SetCustomAdministratorHttpPort( adminHttpPort);
		if (fStartupParameters->GetAdministratorSSLPort( adminSSLPort))
			openingParams.SetCustomAdministratorSSLPort( adminSSLPort);
		if (fStartupParameters->GetDebuggerType( dbgType))
			openingParams.SetDebuggerType( dbgType);
		openingParams.UpdateStartupParameters( inStartupParameters);

		VMicrosecondsCounter usCounter;
		usCounter.Start();

		if (inStartupParameters != NULL)
		{
			VFile *solutionFile = inStartupParameters->GetSolutionFileToOpen();
			if (solutionFile != NULL)
				LogMessage( L"", eL4JML_Information, L"Opens the solution \"" + solutionFile->GetPath().GetPath() + L"\" for running");
		}

		solution = VRIAServerSolution::OpenSolution( err, inStartupParameters);

		if (err == VE_OK)
		{
			if (solution != NULL)
			{
				if (jobOpenBag != NULL)
					JobCommonBagKeys::solutionName.Set( jobOpenBag, solution->GetName());
				
				if (fSolutionMutex.Lock())
				{
					// sc 14/06/2013, the solution must become the current one as soon as possible to prevent it to be closed from JavaScript code (see VJSSolution::_close() method)
					fSolution = solution;
					fSolutionMutex.Unlock();
				}

				LogMessage( L"", eL4JML_Information, L"Starts \"" + solution->GetName() + L"\" solution");

				// sc 10/02/2010 keep informations about recent opened solutions: save the startup parameters in the solution link file
				if ((inStartupParameters != NULL) && inStartupParameters->GetStoreInLinkFile())
					SaveSolutionStartupParametersToLinkFile( solution->GetName(), *inStartupParameters);

				VSolution *designSolution = solution->GetDesignSolution();
				if (designSolution != NULL)
				{
					designSolution->StartWatchingFileSystem();
					if ((inStartupParameters != NULL) ? inStartupParameters->GetOpenProjectSymbolsTable() : true)
						designSolution->StartUpdatingSymbolTable();
				}

				err = solution->Start();
				if (err != VE_OK)
				{
					// sc 19/01/2011 if the solution started with errors, the solution must be stopped
					designSolution->StopWatchingFileSystem();
					designSolution->StopUpdatingSymbolTable();
					solution->Stop();
				}
			}
		}

		if (err == VE_OK)
		{
			if (testAssert(solution != NULL))
			{
				VString logMsg;
				logMsg.Printf( "\"%S\" solution has been successfully opened for running (duration: %i ms)", &solution->GetName(), usCounter.Stop()/1000);
				LogMessage( L"", eL4JML_Information, logMsg);
			}
		}
		else
		{
			if (solution != NULL)
			{
				solution->Close();

				if (fSolutionMutex.Lock())
				{
					fSolution = NULL;
					fSolutionMutex.Unlock();
				}
				solution->Release();
			}
		}

		if (err != VE_OK && inStartupParameters != NULL)
		{
			VFile *solutionFile = inStartupParameters->GetSolutionFileToOpen();
			if (solutionFile != NULL)
				err = vThrowError( VE_RIA_CANNOT_OPEN_SOLUTION_FOR_RUNNING, solutionFile->GetPath().GetPath());
		}

		if ((fSolution == NULL) && openingParams.GetOpenDefaultSolutionWhenOpeningFails())
		{
			VSolutionStartupParameters *startupParams  = RetainDefaultSolutionStartupParameters();
			OpenSolutionAsCurrentSolution( startupParams, inJob);
			QuickReleaseRefCountable( startupParams);
			terminateJob = false;
		}
		
		if (fSolution != NULL)
			_PublishServiceRecord(DEFAULT_SERVICE_NAME);

		--fIsOpeningOrClosingCurrentSolution;
	}
	else
	{
		err = vThrowError( VE_RIA_CURRENT_SOLUTION_ALREADY_BEING_OPENED);
	}

	if (inJob != NULL)
	{
		if (jobOpenBag != NULL)
		{
			JobCommonBagKeys::jobResult.Set( jobOpenBag, (err == VE_OK) ? 0 : 1);
			ILoggerBagKeys::message.Set( jobOpenBag, (err == VE_OK) ? L"solution opened" : L"cannot open the solution");
			if (err == VE_OK)
			{
				IRemoteDebuggerServer *dbgrSrv = VJSGlobalContext::GetDebuggerServer();
				WAKDebuggerType_t dbgrType = (dbgrSrv != NULL) ? dbgrSrv->GetType() : NO_DEBUGGER_TYPE;
				JobCommonBagKeys::currentDebugger.Set( jobOpenBag, DebuggerTypeToDebuggerParam( dbgrType));
			}
		}

		if (terminateJob)
			VRIAServerSupervisor::Get()->TerminateJob( inJob->GetId(), jobOpenBag);
		else
			inJob->Log( jobOpenBag);
	}
	ReleaseRefCountable( &jobOpenBag);

	return err;
}

VError VRIAServerApplication::_CloseCurrentSolution()
{
	xbox_assert(VTaskMgr::Get()->GetCurrentTaskID() == VTaskMgr::Get()->GetMainTask()->GetID());
	
	VError err = VE_OK;

	if (fIsOpeningOrClosingCurrentSolution == 0)	// sc 21/06/2012 reentrance safeguard
	{
		++fIsOpeningOrClosingCurrentSolution;

		// sc 21/06/2012, to avoid dead locks, never lock the solution mutex during the solution stopping.
		if (fSolution != NULL)
		{
			LogMessage( L"", eL4JML_Information, L"Stops \"" + fSolution->GetName() + L"\" solution");

			VSolution *designSolution = fSolution->GetDesignSolution();
			if (designSolution != NULL)
			{
				designSolution->StopWatchingFileSystem();
				designSolution->StopUpdatingSymbolTable();
			}

			err = fSolution->Stop();
			xbox_assert(err == VE_OK);
		}

		if (fSolutionMutex.Lock())
		{
			if (fSolution != NULL)
			{
				LogMessage( L"", eL4JML_Information, L"Closes \"" + fSolution->GetName() + L"\" solution");

				err = fSolution->Close();
				_WithdrawServiceRecord(DEFAULT_SERVICE_NAME, fSolution->GetName());
				xbox_assert(err == VE_OK);
				ReleaseRefCountable( &fSolution);
			}

			fSolutionMutex.Unlock();
		}
		
		--fIsOpeningOrClosingCurrentSolution;
	}
	else
	{
		err = vThrowError( VE_RIA_CURRENT_SOLUTION_ALREADY_BEING_CLOSED);
	}

	return err;
}


#if WITH_SANDBOXED_PROJECT

VError VRIAServerApplication::_OpenServerAdminProject()
{
	VError err = VE_OK;

	VFolder *folder = RetainApplicationResourcesFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath( path);
		path.ToSubFolder( L"Default Solution").ToSubFolder( L"Admin");
		path.SetFileName( L"ServerAdmin", false);
		path.SetExtension( RIAFileKind::kProjectFileExtension);

		VFile *file = new VFile( path);
		if (file != NULL && file->Exists())
		{
			VRIAServerProjectOpeningParameters *openingParams = new VRIAServerProjectOpeningParameters();
			if (openingParams != NULL)
			{
				openingParams->SetOpeningMode( ePOM_FOR_RUNNING | ePOM_SANDBOXED);
				openingParams->SetHandlesServerSupervisor( true);
				openingParams->SetHandlesDebuggerServer( true);
				openingParams->SetCreateAdminUser( true);

				WAKDebuggerType_t dbgType = NO_DEBUGGER_TYPE;
				if (fStartupParameters->GetDebuggerType( dbgType))
					openingParams->SetDebuggerType( dbgType);

				sLONG port = 0;
				if (fStartupParameters->GetAdministratorHttpPort( port))
					openingParams->SetCustomHttpPort( port);
				if (fStartupParameters->GetAdministratorSSLPort( port))
					openingParams->SetCustomSSLPort( port);

				openingParams->SetBreakpointsFolderPath( fConfigurationFolderPath);
				openingParams->SetDebuggerSourcesRootPath( fConfigurationFolderPath);


				VRIAServerProject *project = OpenAndRetainProject( err, *file, openingParams);
				if (project != NULL)
				{
					if (err == VE_OK)
					{
						err = StartProject( project);
						if (err != VE_OK)
							StopProject( project);
					}

					if (err != VE_OK)
						CloseProject( project);

					ReleaseRefCountable( &project);
				}
				openingParams->Release();
			}
			file->Release();
		}
		folder->Release();
	}

	return err;
}


VError VRIAServerApplication::_CloseAllProjects()
{
	for (VectorOfApplication::iterator iter = fProjectsCollection.begin() ; iter != fProjectsCollection.end() ; ++iter)
	{
		(*iter)->Stop();
	}

	VRIAServerJSContextMgr *jsContextMgr = VRIAServerApplication::Get()->GetJSContextMgr();
	fJSContextMgr->BeginPoolsCleanup();
	fJSContextMgr->CleanAllPools( 5000, NULL, NULL);
	fJSContextMgr->EndPoolsCleanup();

	for (VectorOfApplication::iterator iter = fProjectsCollection.begin() ; iter != fProjectsCollection.end() ; ++iter)
	{
		VProject *designProject = RetainRefCountable( (*iter)->GetDesignProject());
	
		(*iter)->Close();

		if (testAssert( designProject != NULL))
		{
			designProject->Unload();
			designProject->Release();
		}
	}

	fProjectsCollection.clear();

	return VE_OK;
}


#endif


void VRIAServerApplication::_FlushDataCache()
{
	xbox_assert(VTaskMgr::Get()->GetCurrentTaskID() == VTaskMgr::Get()->GetMainTask()->GetID());

	if (fDataCacheMutex.Lock())
	{
		if (fDataCacheFlushEnabled)
		{
			if (fComponent_DB4D != NULL)
				fComponent_DB4D->FlushCache( false);

			VDataCacheFlushMessage *msg = new VDataCacheFlushMessage( this);
			if (msg != NULL)
			{
				msg->PostTo( VTaskMgr::Get()->GetMainTask(), false, fDataCacheFlushDelay);
				msg->Release();
			}
		}
		fDataCacheMutex.Unlock();
	}
}

void VRIAServerApplication::_PublishServiceRecord (const XBOX::VString &inServiceName)
{
	if (fPublishMutex.TryToLock())
	{
		if (NULL != fSolution)
		{
			XBOX::VValueBag		valueBag;

			// MD5 sum of solution path
			VValueBag::StKey	solutionPathKey(CVSTR("solutionPosixPathHash"));
			VFilePath			solutionFilePath;
			VString				solutionPosixPath, solutionPosixPathHash;

			fSolution->GetDesignSolution()->GetSolutionFilePath(solutionFilePath);
			solutionFilePath.GetPosixPath(solutionPosixPath);
			VChecksumMD5::GetChecksumFromStringUTF8Hexa(solutionPosixPath, solutionPosixPathHash);
			valueBag.SetString(solutionPathKey, solutionPosixPathHash);

			// tell if server is enterprise
			VValueBag::StKey	serverEnterpriseKey(CVSTR("license"));
			VString				strEnterprise;

			if ( IsEnterpriseVersion() )
				strEnterprise = "enterprise";
			else
				strEnterprise = "community";

			valueBag.SetString(serverEnterpriseKey, strEnterprise);

			// give server version
			VValueBag::StKey	serverVersionKey(CVSTR("version"));
			VString				strVersion( K_CURRENT_PROTOCOL_VERSION );

			valueBag.SetString(serverVersionKey, strVersion);


			// Calculate admin project IP and port
			VectorOfApplication appCollection;
			VString				adminIP, adminPattern, adminHostName, publishName;
			sLONG				adminPort = 0;
			sLONG				adminSSLPort = 0;
			bool				allowSSL = false, SSLMandatory = false;

			fSolution->GetApplications(appCollection);
			for (VectorOfApplication_iter iter = appCollection.begin() ; iter != appCollection.end() ; ++iter)
			{
				if ((*iter)->IsAdministrator())
					(*iter)->GetPublicationSettings( adminHostName, adminIP, adminPort, adminSSLPort, adminPattern, publishName, allowSSL, SSLMandatory);
			}

			valueBag.SetString("path", "index.html");
			VString adminSSLPortStr;
			adminSSLPortStr.FromLong( adminSSLPort );
			valueBag.SetString("_SSLPORT", adminSSLPortStr );	
							
			// Provider name is actually the solution name.

			VString	providerName;
			
			fSolution->GetName(providerName);
			if (providerName.IsEmpty())

				providerName = DEFAULT_NAME;

			// Retrieve default IPv4 and IPv6 address(es) (first in address list).

			VNetAddressList					addressList;
			VNetAddressList::const_iterator	addressIt;
			XBOX::VString					defaultIPv4, defaultIPv6;
			sLONG							version;

			addressList.FromLocalInterfaces();
			for (addressIt = addressList.begin(); addressIt != addressList.end(); addressIt++) {

				if (addressIt->IsV4()) {

					if (!defaultIPv4.GetLength()) {
						
						defaultIPv4 = addressIt->GetIP(&version);
						xbox_assert(version == 4);

					}

				} else {

					xbox_assert(addressIt->IsV6());
					if (!defaultIPv6.GetLength()) {
						
						defaultIPv6 = addressIt->GetIP(&version);
						xbox_assert(version == 6);

					}

				}

			}

			xbox_assert(defaultIPv4.GetLength() || defaultIPv6.GetLength());

			// Publish service records.

			if (adminPort) {

				bool	mainServiceRecordSent;

				mainServiceRecordSent = false;
				for (addressIt = addressList.begin(); addressIt != addressList.end(); addressIt++) {

					// Ignore loopback.

					if (addressIt->IsLoopBack())

						continue;

					XBOX::VString	address;

					if (addressIt->IsV4()) {

						address = addressIt->GetIP(&version);
						xbox_assert(version == 4);

					} else {

						xbox_assert(addressIt->IsV6());
						address = addressIt->GetIP(&version);
						xbox_assert(version == 6);

					}

					XBOX::VServiceRecord	serviceRecord;

					// Trying to send IPv4 and IPv6 address(es) of "main" (default address) service record at once.
					// Ignore them afterwards.

					if (address.EqualToString(defaultIPv4) || address.EqualToString(defaultIPv6)) {

						if (mainServiceRecordSent) 

							continue;

						else {

							serviceRecord.fIPv4Address = defaultIPv4;
							serviceRecord.fIPv6Address = defaultIPv6;

						}

					} else if (version == 4) {

						xbox_assert(!address.IsEmpty());

						serviceRecord.fIPv4Address = address;

					} else {

						xbox_assert(version == 6 && !address.IsEmpty());

						serviceRecord.fIPv6Address = address;

					}
					
					serviceRecord.fPort = adminPort;
					serviceRecord.fServiceName = inServiceName;
					serviceRecord.fProviderName = publishName.GetLength() ? publishName : providerName;

					serviceRecord.SetHostName();
					if (providerName.EqualToString("DefaultSolution", true)) {

						XBOX::VString	suffix;

						serviceRecord.fHostName.GetSubString(1, serviceRecord.fHostName.GetLength() - 6, suffix);
						serviceRecord.fProviderName.AppendString("-");							
						serviceRecord.fProviderName.AppendString(suffix);

					}

					serviceRecord.fValueBag.UnionClone(&valueBag);

					fServiceDiscoveryServer->AddServiceRecord(serviceRecord);		
					fServiceDiscoveryServer->PublishServiceRecord(serviceRecord.fServiceName, serviceRecord.fProviderName);

					if (!mainServiceRecordSent 
					&& (address.EqualToString(defaultIPv4) || address.EqualToString(defaultIPv6))) {

						mainServiceRecordSent = true;

						// DB index and flush infos
						VLong indexProgress, indexSessions, flushProgress, flushSessions;

						GetProgressInfos(RIASERVER_PROGRESS_INDEX_USERINFO, indexProgress, indexSessions);
						serviceRecord.fValueBag.SetLong("_INDEX", indexProgress);
						serviceRecord.fValueBag.SetLong("_INDEX_SESSIONS", indexSessions);
						GetProgressInfos(RIASERVER_PROGRESS_FLUSH_USERINFO, flushProgress, flushSessions);
						serviceRecord.fValueBag.SetLong("_FLUSH", flushProgress);
						serviceRecord.fValueBag.SetLong("_FLUSH_SESSIONS", flushSessions);

						// Address and port.

						VNetAddress	netAddress(serviceRecord.fIPv4Address);
						uLONG		binaryAddress;

						netAddress.FillIpV4((IP4 *) &binaryAddress);

						serviceRecord.fValueBag.SetLong("_ADDRESS", binaryAddress);	
						serviceRecord.fValueBag.SetString("_ADDRESS_IPV6", serviceRecord.fIPv6Address );

						VString adminPortStr;
						adminPortStr.FromLong( serviceRecord.fPort );
						serviceRecord.fValueBag.SetString("_PORT", adminPortStr );	
						serviceRecord.fValueBag.SetString("_NAME", serviceRecord.fProviderName);

						// Storing server executable path
						VFolder*	appFolder = VProcess::Get()->RetainFolder( VProcess::eFS_Bundle);
						VFilePath	exeFilePath = appFolder->GetPath();
						
#if VERSIONWIN
						exeFilePath = exeFilePath.ToSubFile( CVSTR( "Wakanda Server.exe" ) );
#endif
						serviceRecord.fValueBag.SetString("_EXE_PATH", exeFilePath.GetPath() );
						appFolder->Release();

						// Writing publish file
						VFileStream	*fileStream;
						VFile		*file;

						if ((file = VSolution::RetainRunningServerFile()) != NULL)
						{
							file->Create(FCR_Overwrite);
							if ((fileStream = new XBOX::VFileStream(file)) != NULL)
							{
								fileStream->OpenWriting();					
								valueBag.WriteToStream(fileStream);				
								fileStream->CloseWriting(true);
								delete fileStream;
							}				
							delete file;
						}

					}

				}

			}

		}

		fPublishMutex.Unlock();
	}
}

void VRIAServerApplication::_WithdrawServiceRecord (const XBOX::VString &inServiceName, const XBOX::VString &inProviderName)
{
	VString	providerName( inProviderName );
	VFile	*file;

	if (NULL != fSolution)
	{
		if (providerName.IsEmpty())
			providerName = DEFAULT_NAME;

		if (providerName.EqualToString("DefaultSolution", true))
		{
			VServiceRecord serviceRecord;
			serviceRecord.SetHostName();
			XBOX::VString	suffix;

			serviceRecord.fHostName.GetSubString(1, serviceRecord.fHostName.GetLength() - 6, suffix);
			providerName.AppendString("-");							
			providerName.AppendString(suffix);
		}				

		fServiceDiscoveryServer->RemoveServiceRecord(inServiceName, providerName);	//** Check successful completion.
		
		if ((file = VSolution::RetainRunningServerFile()) != NULL)
		{	
			file->Delete();
			delete file;
		}
	}
}


void VRIAServerApplication::_CleanupUserCacheFolder()
{
	VFolder *cacheFolder = VProcess::Get()->RetainProductSystemFolder( eFK_UserCache, false);
	if (cacheFolder != NULL)
	{
		StErrorContextInstaller errorContext( false, true);
		bool doCleanup = true;
		VTime nowTime;
		VTime::Now( nowTime);
		
		VFile lastCleanupFile( *cacheFolder, L"lastCleanup");
		if (lastCleanupFile.Exists())
		{
			VTime modificationTime;
			if (lastCleanupFile.GetTimeAttributes( &modificationTime) == VE_OK)
			{
				VDuration diff;
				nowTime.Substract( modificationTime, diff);
				doCleanup = diff.GetDay() > 7;
			}
		}

		if (doCleanup)
		{
			fputs_VString( L"Please wait, Wakanda Server cleans its cache folder...\n\n", stdout);

			VDuration lDuration;
			lDuration.SetDuration( 30, 0, 0, 0, 0);
			VTime lDate( nowTime);
			lDate.Substract( lDuration);

			std::vector< VRefPtr < VFile > > files;
			std::vector< VRefPtr < VFolder > > folders;

			cacheFolder->GetContents( files, folders);

			for (std::vector< VRefPtr < VFolder > >::iterator foldersIter = folders.begin() ; foldersIter != folders.end() ; ++foldersIter)
			{
				if (FolderContentWasChangedSinceDate( *foldersIter, lDate))
					foldersIter->Set( NULL); // keep this folder
			}			

			for (std::vector< VRefPtr < VFolder > >::iterator foldersIter = folders.begin() ; foldersIter != folders.end() ; ++foldersIter)
			{
				if (!foldersIter->IsNull())
					(*foldersIter)->Delete( true);
			}			

			if (!lastCleanupFile.Exists())
				lastCleanupFile.Create( FCR_Overwrite);
			lastCleanupFile.SetTimeAttributes( &nowTime);
		}
		
		cacheFolder->Release();
	}
}


void VRIAServerApplication::PublishServiceRecordEvent ()
{
	_PublishServiceRecord(DEFAULT_SERVICE_NAME);
}

void VRIAServerApplication::GetProgressInfos(const VString& inServiceName, VLong& outProgressInfos, VLong& outProgressSessions)
{
	VString serviceName(inServiceName);
	VValueBag bag;

	outProgressInfos = -1;
	outProgressSessions = -1;

	VProgressManager::GetProgressInfo(bag, &serviceName);

	if ( ! bag.IsEmpty() )
	{
		VBagArray *progressBag = bag.GetElements("ProgressInfo");
		if (progressBag)
		{
			VValueBag *resultValueBag = progressBag->GetNth(1);
			if (resultValueBag)
			{
				resultValueBag->GetAttribute("percent", outProgressInfos);
				resultValueBag->GetAttribute("sessions", outProgressSessions);
			}
		}
	}
}


bool VRIAServerApplication::GetDebuggingAuthorized() const
{
	if (fStartupParameters != NULL)
	{
		bool debuggingAuthorized = false;
		if (fStartupParameters->GetDebuggingAuthorized( debuggingAuthorized))
			return debuggingAuthorized;
	}

	return true;
}



// ----------------------------------------------------------------------------



VRIAServerJSRuntimeDelegate::VRIAServerJSRuntimeDelegate( XBOX::VFolder *inScriptFolder)
{
	fScriptFolder = RetainRefCountable( inScriptFolder);
}


VRIAServerJSRuntimeDelegate::~VRIAServerJSRuntimeDelegate()
{
	ReleaseRefCountable( &fScriptFolder);
}


VFolder* VRIAServerJSRuntimeDelegate::RetainScriptsFolder()
{
	return RetainRefCountable( fScriptFolder);
}


VProgressIndicator* VRIAServerJSRuntimeDelegate::CreateProgressIndicator( const VString& inTitle)
{
	return NULL;
}


XBOX::VFileSystemNamespace* VRIAServerJSRuntimeDelegate::RetainRuntimeFileSystemNamespace()
{
	return RetainRefCountable( VRIAServerApplication::Get()->GetFileSystemNamespace());
}



// ----------------------------------------------------------------------------


#if WITH_SANDBOXED_PROJECT

VRIAServerConfiguration::VRIAServerConfiguration()
{
}


VRIAServerConfiguration::~VRIAServerConfiguration()
{
}


VError VRIAServerConfiguration::Load( const VFile& inFile)
{
	VError err = VE_OK;

	fConfiguration.SetNull();
	
	if (inFile.Exists())
	{
		err = VJSONImporter::ParseFile( &inFile, fConfiguration, VJSONImporter::EJSI_Strict);
	}
	else
	{
		err = vThrowError( VE_FILE_NOT_FOUND);
	}

	return err;
}

	
bool VRIAServerConfiguration::GetName( VString& outName) const
{
	bool done = false;
	if (fConfiguration.IsObject())
	{
		VJSONValue conf = fConfiguration.GetObject()->GetProperty( L"configuration");
		if (conf.IsObject())
			done = conf.GetObject()->GetPropertyAsString( L"name", outName);
	}
	return done;
}


bool VRIAServerConfiguration::GetFolderPath( VFilePath& outPath) const
{
	bool done = false;
	if (fConfiguration.IsObject())
	{
		VJSONValue conf = fConfiguration.GetObject()->GetProperty( L"configuration");
		if (conf.IsObject())
		{
			VString path;
			done = conf.GetObject()->GetPropertyAsString( L"path", path);
			if (done)
			{
				VURL::Decode( path);
				outPath.FromFullPath( path, FPS_POSIX);
			}
		}
	}
	return done;
}


void VRIAServerConfiguration::GetProjects( std::vector<VFilePath>& outProjectFilePaths) const
{
	outProjectFilePaths.clear();
	if (fConfiguration.IsObject())
	{
		VJSONValue projects = fConfiguration.GetObject()->GetProperty( L"projects");
		if (projects.IsArray())
		{
			VJSONArray *arr = projects.GetArray();
			for (size_t iter = 0, len = arr->GetCount() ; iter < len ; ++iter)
			{
				if ((*arr)[iter].IsObject())
				{
					VString path;
					if ((*arr)[iter].GetObject()->GetPropertyAsString( L"path", path))
					{
						VURL::Decode( path);
						outProjectFilePaths.push_back( VFilePath( path, FPS_POSIX));
					}
				}
			}
		}
	}
}

#endif // WITH_SANDBOXED_PROJECT