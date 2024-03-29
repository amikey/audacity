/**********************************************************************

  Audacity: A Digital Audio Editor

  Prefs.h

  Dominic Mazzoni
  Markus Meyer

  Audacity uses wxWidgets' wxFileConfig class to handle preferences.
  In Audacity versions prior to 1.3.1, it used wxConfig, which would
  store the prefs in a platform-dependent way (e.g. in the registry
  on Windows). Now it always stores the settings in a configuration file
  in the Audacity Data Directory.

  Every time we read a preference, we need to specify the default
  value for that preference, to be used if the preference hasn't
  been set before.

  So, to avoid code duplication, we provide functions in this file
  to read and write preferences which have a nonobvious default
  value, so that if we later want to change this value, we only
  have to change it in one place.

  See Prefs.cpp for a (complete?) list of preferences we keep
  track of...

**********************************************************************/
#ifndef __AUDACITY_PREFS__
#define __AUDACITY_PREFS__

#include "Audacity.h"

#include "../include/audacity/ComponentInterface.h"

#include <memory>
#include <wx/fileconf.h>  // to inherit wxFileConfig
#include <wx/event.h> // to declare custom event types

void InitPreferences();
bool CheckWritablePreferences();
wxString UnwritablePreferencesErrorMessage();
void FinishPreferences();

class AudacityPrefs;


extern AUDACITY_DLL_API AudacityPrefs *gPrefs;
extern int gMenusDirty;


/// \brief Our own specialisation of wxFileConfig.  It is essentially a renaming,
/// though it does provide one new access function.  Most of the prefs work
/// is actually done by the InitPreferences() function.
class  AUDACITY_DLL_API AudacityPrefs : public wxFileConfig 
{
public:
   AudacityPrefs(const wxString& appName = {},
               const wxString& vendorName = {},
               const wxString& localFilename = {},
               const wxString& globalFilename = {},
               long style = wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE,
               const wxMBConv& conv = wxConvAuto());
   bool GetEditClipsCanMove();

   // Set and Get values of the version major/minor/micro keys in audacity.cfg when Audacity first opens
   void SetVersionKeysInit( int major, int minor, int micro)
   {
      mVersionMajorKeyInit = major;
      mVersionMinorKeyInit = minor;
      mVersionMicroKeyInit = micro;
   }
   void GetVersionKeysInit( int& major, int& minor, int& micro) const
   {
      major = mVersionMajorKeyInit;
      minor = mVersionMinorKeyInit;
      micro = mVersionMicroKeyInit;
   }

   // values of the version major/minor/micro keys in audacity.cfg
   // when Audacity first opens
   int mVersionMajorKeyInit{};
   int mVersionMinorKeyInit{};
   int mVersionMicroKeyInit{};
};

/// Packages a table of user-visible choices each with an internal code string,
/// a preference key path, and a default choice
class ChoiceSetting
{
public:
   ChoiceSetting(
      const wxString &key,
      const EnumValueSymbol symbols[], size_t nSymbols,
      size_t defaultSymbol
   )
      : mKey{ key }

      , mSymbols{ symbols }
      , mnSymbols{ nSymbols }

      , mDefaultSymbol{ defaultSymbol }
   {
      wxASSERT( defaultSymbol < nSymbols );
   }

   const wxString &Key() const { return mKey; }
   const EnumValueSymbol &Default() const
      { return mSymbols[mDefaultSymbol]; }
   const EnumValueSymbol *begin() const { return mSymbols; }
   const EnumValueSymbol *end() const { return mSymbols + mnSymbols; }

   wxString Read() const;
   bool Write( const wxString &value ); // you flush gPrefs afterward

protected:
   size_t Find( const wxString &value ) const;
   virtual void Migrate( wxString& );

   const wxString mKey;

   const EnumValueSymbol *mSymbols;
   const size_t mnSymbols;

   // stores an internal value
   mutable bool mMigrated { false };

   const size_t mDefaultSymbol;
};

/// Extends ChoiceSetting with a corresponding table of integer codes
/// (generally not equal to their table positions),
/// and optionally an old preference key path that stored integer codes, to be
/// migrated into one that stores internal string values instead
class EnumSetting : public ChoiceSetting
{
public:
   EnumSetting(
      const wxString &key,
      const EnumValueSymbol symbols[], size_t nSymbols,
      size_t defaultSymbol,

      const int intValues[] = nullptr, // must have same size as symbols
      const wxString &oldKey = wxString("")
   )
      : ChoiceSetting{ key, symbols, nSymbols, defaultSymbol }
      , mIntValues{ intValues }
      , mOldKey{ oldKey }
   {
      wxASSERT( mIntValues );
   }

   // Read and write the encoded values
   virtual int ReadInt() const;
   bool WriteInt( int code ); // you flush gPrefs afterward

protected:
   size_t FindInt( int code ) const;
   void Migrate( wxString& ) override;

private:
   const int *mIntValues;
   const wxString mOldKey;
};

// An event emitted by the application when the Preference dialog commits
// changes
wxDECLARE_EVENT(EVT_PREFS_UPDATE, wxCommandEvent);

// Invoke UpdatePrefs() when Preference dialog commits changes.
class AUDACITY_DLL_API PrefsListener
{
public:
   PrefsListener();
   virtual ~PrefsListener();

   // Called when all preferences should be updated.
   virtual void UpdatePrefs() = 0;

protected:
   // Called when only selected preferences are to be updated.
   // id is some value generated by wxNewId() that identifies the portion
   // of preferences.
   // Default function does nothing.
   virtual void UpdateSelectedPrefs( int id );

private:
   struct Impl;
   std::unique_ptr<Impl> mpImpl;
};

#endif
