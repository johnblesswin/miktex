/* internal.h: internal definitions                     -*- C++ -*-

   Copyright (C) 2013-2018 Christian Schenk

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA. */

#if defined(MIKTEX_SETUP_SHARED)
#  define MIKTEXSETUPEXPORT MIKTEXDLLEXPORT
#else
#  define MIKTEXSETUPEXPORT
#endif

#define C09CDC45E4B649EDA745DAF436D18309
#include "miktex/Setup/SetupService.h"

#if defined(MIKTEX_WINDOWS)
#  include <comdef.h>
#  include <IntShCut.h>
#  include <shlobj.h>
#endif

#include <cstdarg>

#include <mutex>
#include <set>

#include <miktex/Core/BufferSizes>
#include <miktex/Core/CommandLineBuilder>
#include <miktex/Core/ConfigNames>
#include <miktex/Core/Cfg>
#include <miktex/Core/Directory>
#include <miktex/Core/DirectoryLister>
#include <miktex/Core/File>
#include <miktex/Core/FileStream>
#include <miktex/Core/FileType>
#include <miktex/Core/Paths>
#include <miktex/Core/Process>
#include <miktex/Core/Quoter>
#include <miktex/Core/Registry>
#include <miktex/Core/StreamReader>
#include <miktex/Core/StreamWriter>
#include <miktex/Core/Urls>

#if defined(MIKTEX_WINDOWS)
#  include "miktex/Core/win/winAutoResource.h"
#  include "miktex/Core/win/WindowsVersion.h"
#endif

#include <miktex/Extractor/Extractor>

#include <miktex/PackageManager/PackageManager>

#include <miktex/Trace/TraceStream>

#include <miktex/Util/StringUtil>
#include <miktex/Util/Tokenizer>
#include <miktex/Util/inliners.h>

#define UNIMPLEMENTED() MIKTEX_INTERNAL_ERROR()

#define BEGIN_INTERNAL_NAMESPACE                        \
namespace MiKTeX {                                      \
  namespace Setup {                                     \
    namespace B3CB81AE1C634DFBB208D80FA1A264AE {

#define END_INTERNAL_NAMESPACE                  \
    }                                           \
  }                                             \
}

#define BEGIN_ANONYMOUS_NAMESPACE namespace {
#define END_ANONYMOUS_NAMESPACE }

BEGIN_INTERNAL_NAMESPACE;

#if !defined(UNUSED)
#  if !defined(NDEBUG)
#    define UNUSED(x)
#  else
#    define UNUSED(x) static_cast<void>(x)
#  endif
#endif

#if !defined(UNUSED_ALWAYS)
#  define UNUSED_ALWAYS(x) static_cast<void>(x)
#endif

#define T_(x) MIKTEXTEXT(x)

#define Q_(x) MiKTeX::Core::Quoter<char>(x).GetData()

#define TU_(x) MiKTeX::Util::CharBuffer<char>(x).GetData()
#define UT_(x) MiKTeX::Util::CharBuffer<wchar_t>(x).GetData()

#define UW_(x) MiKTeX::Util::StringUtil::UTF8ToWideChar(x).c_str()
#define WU_(x) MiKTeX::Util::StringUtil::WideCharToUTF8(x).c_str()

#define SETUPSTATICFUNC(type) static type
#define SETUPINTERNALFUNC(type) type
#define SETUPINTERNALVAR(type) type

#define ARRAY_SIZE(buf) (sizeof(buf)/sizeof(buf[0]))

inline void AssertValidBuf(void* lp, size_t n)
{
#if defined(MIKTEX_DEBUG)
  MIKTEX_ASSERT(lp != nullptr);
#if defined(MIKTEX_WINDOWS)
  MIKTEX_ASSERT(!IsBadWritePtr(lp, n));
#endif
#else
  UNUSED(lp);
  UNUSED(n);
#endif
}

inline void AssertValidString(const char* lp, size_t n = 4096)
{
#if defined(MIKTEX_DEBUG)
  MIKTEX_ASSERT(lp != nullptr);
#if defined(MIKTEX_WINDOWS)
  MIKTEX_ASSERT(!IsBadStringPtrA(lp, n));
#endif
#else
  UNUSED(lp);
  UNUSED(n);
#endif
}

class LogFile
{
public:
  void SetCallback(class SetupServiceImpl* service)
  {
    this->pService = service;
  }

public:
  void SetLogFileName(const MiKTeX::Core::PathName& logFileName)
  {
    this->logFileName = logFileName;
  }

public:
  struct ProgressInfo
  {
    MiKTeX::Core::PathName fileName;
    size_t total;
    size_t completed;
  };

public:
  ProgressInfo GetProgressInfo()
  {
    ProgressInfo pi;
    pi.completed = n;
    pi.total = files.size();
    return pi;
  }

public:
  void Process();

private:
  void RemoveFiles();

private:
  void ReadLogFile();

private:
  void AddPackages();

private:
  void AddFile(const MiKTeX::Core::PathName& path);

private:
  MiKTeX::Core::PathName currentFile;

private:
  std::set<MiKTeX::Core::PathName> files;

private:
  size_t n;

#if defined(MIKTEX_WINDOWS)
private:
  void RemoveRegistrySettings();

private:
  struct RegValue
  {
    HKEY hkey;
    std::string strSubKey;
    std::string strValueName;
  };

private:
  std::vector<RegValue> regValues;
#endif

private:
  class SetupServiceImpl* pService = nullptr;

private:
  MiKTeX::Core::PathName logFileName;
};

class SetupServiceImpl :
  public SetupService,
  public MiKTeX::Core::IRunProcessCallback,
  public MiKTeX::Packages::PackageInstallerCallback
{
public:
  virtual ~SetupServiceImpl();

public:
  void Initialize() override;

public:
  SetupOptions GetOptions() override
  {
    return this->options;
  }

public:
  SetupOptions SetOptions(const SetupOptions& options) override;

public:
  void OpenLog() override;

public:
  MiKTeX::Core::PathName CloseLog(bool cancel) override;

public:
  void MIKTEXCEECALL Log(const char* format, ...) override;

public:
  void LogV(const char* format, va_list argList) override;

public:
  void ULogOpen() override;

public:
  void ULogClose(bool finalize) override;

public:
  MiKTeX::Core::PathName GetULogFileName() override;

public:
  void ULogAddFile(const MiKTeX::Core::PathName& path) override;

public:
  ProgressInfo GetProgressInfo() override;

public:
  void SetCallback(SetupServiceCallback* callback) override;

public:
  void SetCallbacks(std::function<void(const std::string&)> f_ReportLine, std::function<bool(const std::string&)> f_OnRetryableError, std::function<bool(MiKTeX::Setup::Notification)> f_OnProgress, std::function<bool(const void*, size_t)> f_OnProcessOutput) override;

public:
  void Run() override;

public:
  bool IsCancelled()
  {
    return cancelled;
  }

protected:
  virtual void RegisterUninstaller()
  {
  }

protected:
  virtual void UnregisterPath(bool shared)
  {
  }

protected:
  virtual void UnregisterShellFileTypes()
  {
  };

protected:
  virtual void RemoveRegistryKeys()
  {
  }

public:
  SetupServiceImpl();

protected:
  void CompleteOptions(bool allowRemoteCalls);

protected:
  void DoTheDownload();

protected:
  void DoPrepareMiKTeXDirect();

protected:
  void DoTheInstallation();

protected:
  void DoFinishSetup();

protected:
  void DoFinishUpdate();

protected:
  void DoCleanUp();

protected:
  bool OnProcessOutput(const void* output, size_t n) override;

protected:
  void ReportLine(const std::string& str) override;

protected:
  bool OnRetryableError(const std::string& message) override;

protected:
  bool OnProgress(MiKTeX::Packages::Notification nf) override;

protected:
  SetupOptions options;

protected:
  enum Section { None, Files, HKLM, HKCU };

protected:
  Section section;

protected:
  MiKTeX::Core::StreamWriter logStream;

protected:
  std::mutex logStreamMutex;

protected:
  MiKTeX::Core::PathName intermediateLogFile;

protected:
  MiKTeX::Core::StreamWriter uninstStream;

protected:
  std::unique_ptr<MiKTeX::Trace::TraceStream> traceStream;

protected:
  MiKTeX::Core::PathName GetInstallRoot() const;

protected:
  MiKTeX::Core::PathName GetBinDir() const;

protected:
  void ConfigureMiKTeX();

protected:
  void RunIniTeXMF(const std::vector<std::string>& args);

protected:
  void RunMpm(const std::vector<std::string>& args);

protected:
  std::wstring& Expand(const char* source, std::wstring& dest);

protected:
  bool FindFile(const MiKTeX::Core::PathName& fileName, MiKTeX::Core::PathName& result);

protected:
  void RemoveFormatFiles();

protected:
  void CollectFiles(std::vector<MiKTeX::Core::PathName>& vec, const MiKTeX::Core::PathName& dir, const char* lpszExt);

protected:
  void CreateInfoFile();

protected:
  void UnregisterComponents();

protected:
  std::vector<MiKTeX::Core::PathName> GetRoots();

protected:
  void LogHeader();

protected:
  bool logging = false;

protected:
  bool cancelled;

protected:
  bool initialized = false;

protected:
  std::shared_ptr<MiKTeX::Packages::PackageManager> packageManager;

protected:
  std::shared_ptr<MiKTeX::Packages::PackageInstaller> packageInstaller;

protected:
  LogFile logFile;

protected:
  class InternalCallbacks :
    public SetupServiceCallback
  {
  public:
    std::function<void(const std::string&)> f_ReportLine;
  public:
    void ReportLine(const std::string& str) override
    {
      if (f_ReportLine)
      {
        f_ReportLine(str);
      }
    }
  public:
    std::function<bool(const std::string&)> f_OnRetryableError;
  public:
    bool OnRetryableError(const std::string& message) override
    {
      return f_OnRetryableError ? f_OnRetryableError(message) : true;
    }
  public:
    std::function<bool(MiKTeX::Setup::Notification)> f_OnProgress;
  public:
    bool OnProgress(MiKTeX::Setup::Notification nf) override
    {
      return f_OnProgress ? f_OnProgress(nf) : true;
    }
  public:
    std::function<bool(const void*, size_t)> f_OnProcessOutput;
  public:
    bool OnProcessOutput(const void* output, size_t n) override
    {
      return f_OnProcessOutput ? f_OnProcessOutput(output, n) : true;
    }
  } myCallbacks;

protected:
  SetupServiceCallback* callback = &myCallbacks;
};

void RemoveEmptyDirectoryChain(const MiKTeX::Core::PathName& directory);

END_INTERNAL_NAMESPACE;

using namespace MiKTeX::Setup::B3CB81AE1C634DFBB208D80FA1A264AE;
