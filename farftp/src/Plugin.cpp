#include <all_far.h>

#include "Int.h"

// ------------------------------------------------------------------------
// FTPNotify
// ------------------------------------------------------------------------
void FTPNotify::Notify(const FTNNotify* p)
{
	Interface()->Notify(p);
}
// ------------------------------------------------------------------------
// FTPDirList
// ------------------------------------------------------------------------
WORD     FTPDirList::DetectStringType(FTPServerInfo * const Server,char *ListingString, int ListingLength)
{
	return Interface()->DetectStringType(Server,ListingString,ListingLength);
}
WORD     FTPDirList::GetNumberOfSupportedTypes(void)
{
	return Interface()->GetNumberOfSupportedTypes();
}
FTPType* FTPDirList::GetType(WORD Index)
{
	return Interface()->GetType(Index);
}
WORD     FTPDirList::DetectDirStringType(FTPServerInfo * const Server,LPCSTR ListingString)
{
	return Interface()->DetectDirStringType(Server,ListingString);
}

//------------------------------------------------------------------------
FTPInterface Interface;
static BOOL         InterfaceInited = FALSE;

//------------------------------------------------------------------------
HANDLE _cdecl idProcStart(LPCSTR FunctionName,LPCSTR Format,...)
{
	String str;
	va_list argptr;
	va_start(argptr,Format);
	str.vprintf(Format,argptr);
	va_end(argptr);
	return new FARINProc(FunctionName,str.c_str());
}

void           WINAPI idProcEnd(HANDLE proc)
{
	delete((FARINProc*)proc);
}
OptionsPlugin* WINAPI idGetOpt(void)
{
	return &Opt;
}

int WINAPI idFtpCmdBlock(int block /*TRUE,FALSE,-1*/)
{
	FTP *ftp = LastUsedPlugin;

	if(!ftp || !ftp->hConnect) return -1;

	return FtpCmdBlock(ftp->hConnect,block);
}

int WINAPI idFtpGetRetryCount(void)
{
	FTP *ftp = LastUsedPlugin;

	if(!ftp || !ftp->hConnect) return 0;

	return FtpGetRetryCount(ftp->hConnect);
}

FTPHostPlugin* WINAPI idGetHostOpt(void)
{
	FTP *ftp = LastUsedPlugin;

	if(!ftp || !ftp->hConnect) return NULL;

	return &ftp->hConnect->Host;
}

//------------------------------------------------------------------------
void CreateFTPInterface(void)
{
	InterfaceInited = TRUE;
	Interface.Magic           = FTP_INTERFACE_MAGIC;
	Interface.SizeOf          = sizeof(Interface);
	Interface.Info            = FP_Info;
	Interface.FSF             = FP_FSF;
	Interface.PluginRootKey   = FP_PluginRootKey;
	Interface.PluginStartPath = FP_PluginStartPath;

//	Interface.FTPModule       = FP_HModule;
//FAR
	Interface.GetMsg          = FP_GetMsgINT;
	Interface.GetMsgStr       = FP_GetMsgSTR;
//Debug
	Interface.Assertion       = __WinAbort;
	Interface.SayLog          = FARINProc::Say;
	Interface.LogProcStart    = idProcStart;
	Interface.LogProcEnd      = idProcEnd;
//Reg
	Interface.GetRegKeyFullInt  = FP_GetRegKey;
	Interface.GetRegKeyFullStr  = FP_GetRegKey;
	Interface.GetRegKeyInt      = FP_GetRegKey;
	Interface.GetRegKeyStr      = FP_GetRegKey;
//Std
	Interface.StrCmp    = StrCmp;
	Interface.StrCpy    = StrCpy;
	Interface.StrCat    = StrCat;
//Utilities
	Interface.Message          = Message;
	Interface.MessageV         = MessageV;
	Interface.PointToName      = PointToName;
	Interface.FDigit           = FDigit;
	Interface.FCps             = FCps;
	Interface.FMessage         = FMessage;
	Interface.CheckForEsc      = CheckForEsc;
	Interface.IdleMessage      = IdleMessage;
//FTP related
	Interface.FtpGetRetryCount = idFtpGetRetryCount;
	Interface.FtpCmdBlock      = idFtpCmdBlock;
//Info
	Interface.GetOpt           = idGetOpt;
	Interface.GetHostOpt       = idGetHostOpt;
}

FTPPluginHolder* StdCreator(FTPPluginInterface* Interface)
{
	FTPPluginHolder* p = new FTPPluginHolder;

	if(!p->Assign(Interface))
	{
		delete p;
		return NULL;
	}

	return p;
}


FTPPluginInterface* WINAPI FTPPluginGetInterface_Progress(void);
FTPPluginInterface* WINAPI FTPPluginGetInterface_DirList(void);
FTPPluginInterface* WINAPI FTPPluginGetInterface_Notify(void);

struct FTPPluginsInfo
{
	DWORD            Magic;
	FTPPluginInterface* interface;
	FTPPluginHolder* Holder;
	LPCSTR         Description;
} StdPlugins[] =
{

	/*PLUGIN_xxx*/
	/*PLUGIN_PROGRESS*/ { FTP_PROGRESS_MAGIC, FTPPluginGetInterface_Progress(), NULL, FMSG("Ftp plugin progress dialog") },
	/*PLUGIN_DIRLIST*/  { FTP_DIRLIST_MAGIC,  FTPPluginGetInterface_DirList(), NULL, FMSG("Ftp plugin directory listing parcer") },
	/*PLUGIN_NOTIFY*/   { FTP_NOTIFY_MAGIC,   FTPPluginGetInterface_Notify(), NULL, NULL },

	{ 0,NULL,NULL,NULL }
};

//------------------------------------------------------------------------
BOOL InitPlugins(void)
{
	CreateFTPInterface();
	for(int n = 0; StdPlugins[n].Magic; n++)
	{
		StdPlugins[n].Holder = StdCreator(StdPlugins[n].interface);
	}
	return TRUE;
}

void FreePlugins(void)
{
	if(InterfaceInited)
	{
		InterfaceInited = FALSE;

		for(int n = 0; StdPlugins[n].Magic; n++)
			if(StdPlugins[n].Holder)
			{
				StdPlugins[n].Holder->Destroy();
				delete StdPlugins[n].Holder;
				StdPlugins[n].Holder = NULL;
			}
	}
}


FTPPluginHolder* GetPluginHolder(WORD Number)
{
	Assert(Number < ARRAYSIZE(StdPlugins)-1);
	return StdPlugins[Number].Holder;
}

BOOL PluginAvailable(WORD Number)
{
	return Number < ARRAYSIZE(StdPlugins)-1 &&
	       StdPlugins[Number].Holder;
}

//------------------------------------------------------------------------
BOOL FTPPluginHolder::Assign(FTPPluginInterface* inf)
{
	Interface = inf;
	return TRUE;
}
void FTPPluginHolder::Destroy(void)
{
	Interface = NULL;
}
//------------------------------------------------------------------------
#define CH_OBJ if (!Object) Object = Interface()->CreateObject();

void FTPProgress::Resume(LPCSTR LocalFileName)
{
	CH_OBJ Interface()->ResumeFile(Object,LocalFileName);
}
void FTPProgress::Resume(int64_t size)
{
	CH_OBJ Interface()->Resume(Object,size);
}
BOOL FTPProgress::Callback(int Size)
{
	CH_OBJ return Interface()->Callback(Object,Size);
}
void FTPProgress::Init(HANDLE Connection,int tMsg,int OpMode,FP_SizeItemList* il)
{
	CH_OBJ Interface()->Init(Object,Connection,tMsg,OpMode,il);
}
void FTPProgress::Skip(void)
{
	CH_OBJ Interface()->Skip(Object);
}
void FTPProgress::Waiting(time_t paused)
{
	CH_OBJ Interface()->Waiting(Object,paused);
}
void FTPProgress::SetConnection(HANDLE Connection)
{
	CH_OBJ Interface()->SetConnection(Object,Connection);
}

void FTPProgress::InitFile(PluginPanelItem *pi, LPCSTR SrcName, LPCSTR DestName)
{
	int64_t sz = pi ? pi->FindData.nFileSize : 0;

	InitFile(sz, SrcName, DestName);
}

void FTPProgress::InitFile(FAR_FIND_DATA* pi, LPCSTR SrcName, LPCSTR DestName)
{
	int64_t sz = pi ? pi->nFileSize : 0;

	InitFile(sz, SrcName, DestName);
}

void FTPProgress::InitFile(int64_t sz, LPCSTR SrcName, LPCSTR DestName)
{
	CH_OBJ
	Interface()->InitFile(Object,sz,SrcName,DestName);
}
