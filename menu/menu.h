#ifndef _MENU_H_
#define _MENU_H_

#define SYSTEM_DIR		"pocketsnes"

#define ROM_LIST_FILENAME			"romlist.bin"
#define SRAM_FILE_EXT				"srm"
#define SAVESTATE_EXT				"sv"
#define MENU_OPTIONS_FILENAME		"pocketsnes_options"
#define MENU_OPTIONS_EXT			"opt"
#define DEFAULT_ROM_DIR_FILENAME	"romdir"
#define DEFAULT_ROM_DIR_EXT			"opt"

#define SAVESTATE_MODE_SAVE			0
#define SAVESTATE_MODE_LOAD			1
#define SAVESTATE_MODE_DELETE		2

#define POCKETSNES_VERSION			"PocketSNES+ v2"

#define MENU_NORMAL_CPU_SPEED 		0
#define MENU_FAST_CPU_SPEED			0

enum  MENU_ENUM
{
	SAVESTATE_MENU_LOAD,
	SAVESTATE_MENU_SAVE,
	MENU_SETTINGS,
	MENU_RESET_GAME,
	MENU_EXIT_APP,
	MENU_ROM_SELECT,
	MENU_COUNT
};

enum SETTINGS_MENU_ENUM
{
	VIDEO_MENU_SETTINGS = 0,
	AUDIO_MENU_SETTINGS,
	SETTINGS_MENU_CYCLES,
	SETTINGS_MENU_CPU_CLOCK,
	SETTINGS_MENU_AUTO_SAVE_SRAM,
	SETTINGS_MENU_SAVE_CURRENT_SETTINGS,
	SETTINGS_MENU_SAVE_GLOBAL_SETTINGS,
	MENU_CREDITS,
	SETTINGS_MENU_COUNT
};

enum AUDIO_SETTINGS_MENU_ENUM
{
	AUDIO_SETTINGS_MENU_SOUND_ON = 0,
//	AUDIO_SETTINGS_MENU_SOUND_VOL,
	AUDIO_SETTINGS_MENU_SOUND_RATE,
	AUDIO_SETTINGS_MENU_SOUND_STEREO,
	AUDIO_SETTINGS_MENU_SOUND_ECHO,
	AUDIO_SETTINGS_MENU_SOUND_INTERP,
	AUDIO_SETTINGS_MENU_SOUND_SYNC,
	AUDIO_SETTINGS_MENU_COUNT
};

enum VIDEO_SETTINGS_MENU_ENUM
{
	VIDEO_SETTINGS_MENU_FULLSCREEN = 0,
	VIDEO_SETTINGS_MENU_HALFRES,
	VIDEO_SETTINGS_MENU_TRANSPARENCY,
	VIDEO_SETTINGS_MENU_FRAMESKIP,
	VIDEO_SETTINGS_MENU_AUTOFSCAP,
	VIDEO_SETTINGS_MENU_FPS,
	VIDEO_SETTINGS_MENU_COUNT
};

enum SRAM_MENU_ENUM
{
	SRAM_MENU_LOAD = 0,
	SRAM_MENU_SAVE,
	SRAM_MENU_DELETE,
	SRAM_MENU_RETURN,
	SRAM_MENU_COUNT,
};

enum EVENT_TYPES
{
	EVENT_NONE = 0,
	EVENT_EXIT_APP,
	EVENT_LOAD_ROM,
	EVENT_RUN_ROM,
	EVENT_RESET_ROM,
};

enum MENU_MESSAGE_BOX_MODE
{
	MENU_MESSAGE_BOX_MODE_MSG = 0,
	MENU_MESSAGE_BOX_MODE_PAUSE,
	MENU_MESSAGE_BOX_MODE_YESNO,
};

//Graphic size definitions
#define MENU_TILE_WIDTH      64
#define MENU_TILE_HEIGHT     64
#define MENU_HEADER_WIDTH    320
#define MENU_HEADER_HEIGHT   48
#define HIGHLIGHT_BAR_WIDTH  320
#define HIGHLIGHT_BAR_HEIGHT 16

#define INP_BUTTON_MENU_SELECT			SAL_INPUT_A
#define INP_BUTTON_MENU_CANCEL			SAL_INPUT_B
#define INP_BUTTON_MENU_ENTER			SAL_INPUT_SELECT
#define INP_BUTTON_MENU_DELETE			SAL_INPUT_SELECT
#define INP_BUTTON_MENU_PREVIEW_SAVESTATE	SAL_INPUT_Y
#define INP_BUTTON_MENU_QUICKSAVE1		SAL_INPUT_R
#define INP_BUTTON_MENU_QUICKSAVE2		SAL_INPUT_SELECT
#define INP_BUTTON_MENU_QUICKLOAD1		SAL_INPUT_L
#define INP_BUTTON_MENU_QUICKLOAD2		SAL_INPUT_SELECT
#define MENU_TEXT_LOAD_SAVESTATE 		"Press A to load"
#define MENU_TEXT_OVERWRITE_SAVESTATE	"A to overwrite | X to delete"
#define MENU_TEXT_DELETE_SAVESTATE 		"Press X to delete"
#define MENU_TEXT_PREVIEW_SAVESTATE 	"Press Y to preview"


struct MENU_OPTIONS
{
  unsigned int optionsVersion;
  unsigned int frameSkip;
  unsigned int soundEnabled;
  /* The following setting was 'transparency', which is now always enabled.
   * This setting word cannot be reused for any other purpose. It is not
   * guaranteed to be initialised to 0 in most installs of PocketSNES. */
  unsigned int Unused_20140603_1;
  unsigned int volume;
  unsigned int pad_config[32];
  unsigned int country;
  unsigned int showFps;
  unsigned int stereo;
  unsigned int fullScreen;
  unsigned int autoSaveSram;
  unsigned int cpuSpeed;
  unsigned int soundRate;
  unsigned int soundSync;
  unsigned int halfRes;        /* spare02: 0=Full 224p, 1=Half-V Fast, 2=Interlaced */
  unsigned int noTransparency; /* spare03: 0=Transparency ON, 1=OFF */
  unsigned int autoFsCap;      /* spare04: 0=Smooth(Max 1), 1=Balanced(Max 2), 2=Legacy(Max 5) */
  unsigned int disableEcho;    /* spare05: 0=Echo ON, 1=OFF */
  unsigned int disableInterp;  /* spare06: 0=Interp ON, 1=OFF */
  unsigned int cyclesMode;     /* spare07: 0=100%, 1=115%, 2=130%, 3=150%, 4=75%, 5=85% */
  unsigned int clockMhz;       /* spare08: 0=SYSTEM, or 528..678 MHz */
  unsigned int spare09;

};

struct SAVE_STATE
{
  s8 filename[SAL_MAX_PATH];
  s8 fullFilename[SAL_MAX_PATH];
  u32 inUse;
};

//####################################
//# Functions
//####################################
void MenuInit(const char *systemDir, struct MENU_OPTIONS *menuOptions);
s32 MenuRun(s8 *romName);
//void LoadSram(const char *path, const char *romname, const char *ext, const char *srammem);
//void SaveSram(const char *path, const char *romname, const char *ext, const char *srammem);
//void DeleteSram(const char *path, const char *romname, const char *ext);
s32 SaveMenuOptions(const char *path, const char *filename, const char *ext,
			const char *optionsmem, s32 maxSize, s32 showMessage);
s32 LoadMenuOptions(const char *path, const char *filename, const char *ext,
			const char *optionsmem, s32 maxSize, s32 showMessage);
s32 DeleteMenuOptions(const char *path, const char *filename,
			const char *ext, s32 showMessage);
void LoadCurrentOptions(void);
void DefaultMenuOptions(void);
void MenuPause(void);
void PrintTitle(const char *title);
void PrintTile();
void PrintBar(u32 givenY);
s32 MenuMessageBox(const char *message1, const char *message2,
			const char *message3, enum MENU_MESSAGE_BOX_MODE mode);
u32 IsPreviewingState();

bool LoadStateFile(s8 *filename);
bool SaveStateFile(s8 *filename);


#endif /* _MENU_H_ */
