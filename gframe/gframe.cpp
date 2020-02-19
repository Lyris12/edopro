#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <event2/thread.h>
#include <IrrlichtDevice.h>
#include <IGUIButton.h>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIWindow.h>
#include "config.h"
#include "game.h"
#include "data_manager.h"
#include "log.h"
#ifdef __APPLE__
#import <CoreFoundation/CoreFoundation.h>
#include "osx_menu.h"
#endif
#ifdef __ANDROID__
#include "porting_android.h"
#endif

int enable_log = 0;
bool exit_on_return = false;
bool is_from_discord = false;
bool open_file = false;
path_string open_file_name = EPRO_TEXT("");

inline void TriggerEvent(irr::gui::IGUIElement* target, irr::gui::EGUI_EVENT_TYPE type) {
	irr::SEvent event;
	event.EventType = irr::EET_GUI_EVENT;
	event.GUIEvent.EventType = type;
	event.GUIEvent.Caller = target;
	ygo::mainGame->device->postEventFromUser(event);
}

inline void ClickButton(irr::gui::IGUIElement* btn) {
	TriggerEvent(btn, irr::gui::EGET_BUTTON_CLICKED);
}

inline void SetCheckbox(irr::gui::IGUICheckBox* chk, bool state) {
	chk->setChecked(state);
	TriggerEvent(chk, irr::gui::EGET_CHECKBOX_CHANGED);
}

#ifdef UNICODE
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
#ifdef __ANDROID__
	porting::initAndroid();
	porting::initializePathsAndroid();
	if(chdir(porting::working_directory.c_str())!=0)
		LOGE("failed to change directory");
#endif
#ifdef __APPLE__
	CFURLRef bundle_url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFStringRef bundle_path = CFURLCopyFileSystemPath(bundle_url, kCFURLPOSIXPathStyle);
	CFURLRef bundle_base_url = CFURLCreateCopyDeletingLastPathComponent(NULL, bundle_url);
	CFRelease(bundle_url);
	CFStringRef path = CFURLCopyFileSystemPath(bundle_base_url, kCFURLPOSIXPathStyle);
	CFRelease(bundle_base_url);
#ifdef MAC_OS_DISCORD_LAUNCHER
	system(fmt::format("open {}/Contents/MacOS/discord-launcher.app --args random", CFStringGetCStringPtr(bundle_path, kCFStringEncodingUTF8)).c_str());
#endif
	chdir(CFStringGetCStringPtr(path, kCFStringEncodingUTF8));
	CFRelease(path);
	CFRelease(bundle_path);
#endif //__APPLE__
	if(argc >= 2) {
		if(argv[1] == path_string(EPRO_TEXT("from_discord"))) {
			is_from_discord = true;
#if defined(_WIN32)
			SetCurrentDirectory(argv[2]);
#if !defined(_DEBUG)
		} else {
			auto extension = ygo::Utils::GetFileExtension(argv[1]);
			if(extension == EPRO_TEXT("ydk") || extension == EPRO_TEXT("yrp") || extension == EPRO_TEXT("yrpx")) {
				TCHAR exepath[MAX_PATH];
				GetModuleFileName(NULL, exepath, MAX_PATH);
				auto path = ygo::Utils::GetFilePath(exepath);
				SetCurrentDirectory(path.c_str());
			}
#endif //_DEBUG
#endif //_WIN32
		}
	}
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
	evthread_use_windows_threads();
#else
	setlocale(LC_CTYPE, "UTF-8");
	evthread_use_pthreads();
#endif //_WIN32
	ygo::Game _game{};
	ygo::mainGame = &_game;
#ifdef __ANDROID__
	ygo::mainGame->appMain = porting::app_global;
	ygo::mainGame->working_directory = porting::working_directory;
#else
	ygo::mainGame->working_directory = EPRO_TEXT("./");
#endif
	if(!ygo::mainGame->Initialize())
		return EXIT_FAILURE;
#ifdef __APPLE__
	EDOPRO_SetupMenuBar([]() {
		ygo::mainGame->gameConf.fullscreen = !ygo::mainGame->gameConf.fullscreen;
		ygo::mainGame->gSettings.chkFullscreen->setChecked(ygo::mainGame->gameConf.fullscreen);
	});
#endif
	bool keep_on_return = false;
	for(int i = 1; i < argc; ++i) {
		path_string parameter(argv[i]);
#define PARAM_CHECK(x) if(parameter == EPRO_TEXT(x))
#define RUN_IF(x,y) PARAM_CHECK(x) {i++; if(i < argc) {y;} continue;}
#define SET_TXT(elem) ygo::mainGame->elem->setText(ygo::Utils::ToUnicodeIfNeeded(parameter).c_str())
		// Extra database
		RUN_IF("-e", ygo::dataManager.LoadDB(parameter))
		// Nickname
		else RUN_IF("-n", SET_TXT(ebNickName))
		// Host address
		else RUN_IF("-h", SET_TXT(ebJoinHost))
		// Host Port
		else RUN_IF("-p", SET_TXT(ebJoinPort))
		// Host password
		else RUN_IF("-w", SET_TXT(ebJoinPass))
#undef RUN_IF
#undef SET_TXT
		else PARAM_CHECK("-k") { // Keep on return
			exit_on_return = false;
			keep_on_return = true;
		} else PARAM_CHECK("-m") { // Mute
			SetCheckbox(ygo::mainGame->tabSettings.chkEnableSound, false);
			SetCheckbox(ygo::mainGame->tabSettings.chkEnableMusic, false);
		} else PARAM_CHECK("-d") { // Deck
			++i;
			if(i + 1 < argc) { // select deck
				ygo::mainGame->gameConf.lastdeck = ygo::Utils::ToUnicodeIfNeeded(parameter);
				continue;
			} else { // open deck
				exit_on_return = !keep_on_return;
				if(i < argc) {
					open_file = true;
					open_file_name = std::move(parameter);
				}
				ClickButton(ygo::mainGame->btnDeckEdit);
				break;
			}
		} else PARAM_CHECK("-c") { // Create host
			exit_on_return = !keep_on_return;
			ygo::mainGame->HideElement(ygo::mainGame->wMainMenu);
			ClickButton(ygo::mainGame->btnHostConfirm);
			break;
		} else PARAM_CHECK("-j") { // Join host
			exit_on_return = !keep_on_return;
			ygo::mainGame->HideElement(ygo::mainGame->wMainMenu);
			ClickButton(ygo::mainGame->btnJoinHost);
			break;
		} else PARAM_CHECK("-r") { // Replay
			exit_on_return = !keep_on_return;
			++i;
			if(i < argc) {
				open_file = true;
				open_file_name = std::move(parameter);
			}
			ClickButton(ygo::mainGame->btnReplayMode);
			if(open_file)
				ClickButton(ygo::mainGame->btnLoadReplay);
			break;
		} else PARAM_CHECK("-s") { // Single
			exit_on_return = !keep_on_return;
			++i;
			if(i < argc) {
				open_file = true;
				open_file_name = std::move(parameter);
			}
			ClickButton(ygo::mainGame->btnSingleMode);
			if(open_file)
				ClickButton(ygo::mainGame->btnLoadSinglePlay);
			break;
		} else if(argc == 2 && path_string(argv[1]).size() >= 4) {
			auto extension = ygo::Utils::GetFileExtension(parameter);
			if(extension == EPRO_TEXT("ydk")) {
				open_file = true;
				open_file_name = std::move(parameter);
				exit_on_return = !keep_on_return;
				ClickButton(ygo::mainGame->btnDeckEdit);
				break;
			}
			if(extension == EPRO_TEXT("yrp") || extension == EPRO_TEXT("yrpx")) {
				open_file = true;
				open_file_name = std::move(parameter);
				exit_on_return = !keep_on_return;
				ClickButton(ygo::mainGame->btnReplayMode);
				ClickButton(ygo::mainGame->btnLoadReplay);
				break;
			}
		}
#undef ELEM
#undef PARAM_CHECK
	}
	ygo::mainGame->MainLoop();
#ifdef _WIN32
	WSACleanup();
#endif //_WIN32
	return EXIT_SUCCESS;
}
