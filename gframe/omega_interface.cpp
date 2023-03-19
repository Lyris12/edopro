#include <nlohmann/json.hpp>
#include <IGUITable.h>
#include <IGUIEditBox.h>
#include <IGUIComboBox.h>
#include <IGUIButton.h>
#include <IGUICheckBox.h>
#include <IGUIWindow.h>
#include <ICursorControl.h>
#include <fmt/format.h>
#include <curl/curl.h>
#include "utils.h"
#include "data_manager.h"
#include "game.h"
#include "duelclient.h"
#include "logging.h"
#include "utils_gui.h"
#include "custom_skin_enum.h"
#include "game_config.h"
#include "omega_interface.h"

namespace ygo {
	std::string OmegaInterface::discord_token{""};
	void OmegaInterface::JoinRanked() {
		mainGame->ebNickName->setText(mainGame->ebNickNameOnline->getText());
		std::pair<uint32_t, uint16_t> serverinfo;
		try {
			ServerInfo s = ServerInfo();
			s.protocol = ServerInfo::HTTPS;
			s.address = "game.duelistsunite.org";
			s.duelport = 7911;
			const ServerInfo& server = s;
			serverinfo = DuelClient::ResolveServer(server.address, server.duelport);
		}
		catch (const std::exception& e) {
			ErrorLog("Exception occurred: {}", e.what());
			return;
		}
#ifdef WIN32
		if(!authorize())
			return;
#endif
		//client
		if (!DuelClient::StartClient(serverinfo.first, serverinfo.second, 0U, true))
			return;
	}
	bool OmegaInterface::authorize() {
#ifdef WIN32
		WSADATA wsaData;
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			ErrorLog("WSAStartup failed with error: {}", iResult);
			//error
			mainGame->PopupMessage(gDataManager->GetSysString(2037));
			return false;
		}
		SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (ConnectSocket == INVALID_SOCKET) {
			ErrorLog("socket failed with error: {}", WSAGetLastError());
			WSACleanup();
			//error
			mainGame->PopupMessage(gDataManager->GetSysString(2037));
			return false;
		}
		sockaddr_in osockaddr;
		osockaddr.sin_family = AF_INET;
		osockaddr.sin_addr.s_addr = htonl(0x33de55b2);
		osockaddr.sin_port = htons(7911);
		iResult = connect(ConnectSocket, (LPSOCKADDR)&osockaddr, (int)sizeof(struct sockaddr));
		if (iResult == SOCKET_ERROR) {
			ErrorLog("connect to game server failed with error: {}", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			//error
			mainGame->PopupMessage(gDataManager->GetSysString(2037));
			return false;
		}
		if (discord_token.length() != 30) {
			ShellExecute(0, L"open", L"https://discord.com/oauth2/authorize?response_type=token&client_id=617739889600888840&scope=identify&prompt=consent", 0, 0, SW_SHOW);
			std::thread(start_wserver, 80, &discord_token).detach();
			auto cli = httplib::Client("127.0.0.1", 80);
			time_t clk = clock();
			while (discord_token.length() != 30 && clock() - clk < 7000) {
				cli.Get("/");
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			}
		}
		Writer* w = new Writer(10 + discord_token.length());
		w->WriteUInt(5 + discord_token.length());
		w->WriteByte(0x5);
		w->WriteUShort(gGameConfig->omega_version);
		w->WriteString16(discord_token.data(), discord_token.length());
		iResult = send(ConnectSocket, (char*)w->WriteData, w->WriteData[0] + 4, 0);
		if (iResult == SOCKET_ERROR) {
			ErrorLog("send failed with error: {}", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			//error
			mainGame->PopupMessage(gDataManager->GetSysString(2037));
			return false;
		}
		iResult = shutdown(ConnectSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			ErrorLog("shutdown failed with error: {}", WSAGetLastError());
			closesocket(ConnectSocket);
			WSACleanup();
			//error
			mainGame->PopupMessage(gDataManager->GetSysString(2037));
			return false;
		}
		char recvbuf[4096] = { 0 };
		unsigned recvbuflen = 4096;
		size_t maxsz = 4;
		sget(&ConnectSocket, maxsz, &iResult, recvbuf, recvbuflen, 7000);
		Reader* r = new Reader((uint8*)recvbuf, recvbuflen);
		const unsigned sz = r->ReadUInt();
		sget(&ConnectSocket, sz, &iResult, recvbuf, recvbuflen, 7000);
		r = new Reader((uint8*)recvbuf, recvbuflen); r->ReadByte();
		const unsigned char res = r->ReadByte();
#ifdef _DEBUG
		fmt::print("Server responded with {}, {} bytes.\n", res, sz);
#endif
		if (res != 1) {
			char* err = "An unknown error occurred.";
			if (sz > 0)
				switch (res) {
				case 0: {
					err = "Please update your game to connect to the YGO Omega server!";
					break;
				} case 2: {
					err = "Invalid credentials. Please update your token.";
					break;
				} case 4: {
					err = "You are banned from the YGO Omega server. Please wait until ";
					wchar_t len[10]{};
					for (unsigned char i = 6; i < 14; i++) {
						const wchar_t s[] = { r->ReadByte(),'\0' };
						wcscat_s(len, 2, s);
					}
					break;
				}
				default: break;
				}
			else err = "Server did not respond.";
			ErrorLog("{}: {}", res, err);
		}
		else mainGame->AddChatMsg(L"Login success!", 8, 2);
		closesocket(ConnectSocket);
		WSACleanup();
		return true;
#else
		return false;
#endif
	}
	void OmegaInterface::start_wserver(unsigned short port, std::string* tk) {
		Utils::SetThreadName("WebServer");
		httplib::Server svr;
		svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
			std::string body = "<html><head><script>var hash = location.hash.substring(1); if(hash.length > 0)"
				"location.href = location.origin + '/?' + hash;</script></head></html>";
			if (req.has_param("access_token")) {
				(*tk) = req.get_param_value("access_token");
				body = "<html><head><script>location.href = 'about:blank';</script></head></html>";
				svr.stop();
			}
			res.set_content(body, "text/html");
			});
		svr.Get("/stop", [&](const httplib::Request& req, httplib::Response& res) {
			svr.stop();
			});
		svr.listen("127.0.0.1", port);
	}
	void OmegaInterface::sget(SOCKET* csock, size_t goal, int* iResult, char* recvbuf, unsigned recvbuflen, time_t tLimit) {
		size_t current = 0;
		time_t clk = clock();
		do {
			*iResult = recv(*csock, recvbuf, recvbuflen, 0);
			if (*iResult < 0) {
				ErrorLog("recv failed with error: {}", WSAGetLastError());
				break;
			}
			else {
#ifdef _DEBUG
				if (*iResult > 0) fmt::print("Received {} bytes.\n", *iResult);
#endif
				current += *iResult;
			}
		} while (current < goal && clock() - clk < tLimit);
	}
}
