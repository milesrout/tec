// Copyright (c) 2013-2016 Trillek contributors. See AUTHORS.txt for details
// Licensed under the terms of the LGPLv3. See licenses/lgpl-3.0.txt

#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "vfs.hpp"
using tec::VFS;

#include "common/vfs/fs.hpp"
#include "common/vfs/vfs.hpp"
#include "resources/md5mesh.hpp"

#include "server-connection.hpp"
#include "server-message.hpp"
#include "controllers/fps-controller.hpp"
#include "events.hpp"
#include "event-system.hpp"
#include "filesystem.hpp"
#include "gui/console.hpp"
#include "imgui-system.hpp"
#include "lua-system.hpp"
#include "os.hpp"
#include "graphics/view.hpp"
#include "physics-system.hpp"
#include "render-system.hpp"
#include "simulation.hpp"
#include "game-state-queue.hpp"
#include "sound-system.hpp"
#include "vcomputer-system.hpp"
#include "voxel-volume.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace tec {
	extern void InitializeComponents();
	extern void InitializeFileFactories();
	extern void BuildTestEntities();
	extern void ProtoLoad();
	eid active_entity;
}

// This file uses filesystem.hpp purely to get a settings directory path for logging purposes

int main(int argc, char* argv[]) {
	auto loglevel = spdlog::level::info;

	tec::InitializeComponents();
	tec::InitializeFileFactories();
	// TODO write a proper arguments parser
	// Now only search for -v or -vv to set log level
	for (int i = 1; i < argc; i++) {
		if (std::string(argv[i]).compare("-v")) {
			loglevel = spdlog::level::debug;
		}
		else if (std::string(argv[i]).compare("-vv")) {
			loglevel = spdlog::level::trace;
		}
	}
	// Console and logging initialization
	tec::Console console;

	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
	sinks.push_back(std::make_shared<tec::ConsoleSink>(console));
	auto log = std::make_shared<spdlog::logger>("console_log", begin(sinks), end(sinks));
	log->set_level(loglevel);
	log->set_pattern("%v"); // [%l] [thread %t] %v"); // Format on stdout
	spdlog::register_logger(log);

	log->info("Initializing OpenGL...");
	tec::OS os;
	if (!os.InitializeWindow(1024, 768, "TEC 0.1", 4, 0)) {
		log->info("Exiting. The context wasn't created properly please update drivers and try again.");
		exit(1);
	}
	console.AddConsoleCommand(
		"exit",
		"exit : Exit from TEC",
		[&os] (const char*) {
			os.Quit();
		});
	std::thread* asio_thread = nullptr;
	std::thread* sync_thread = nullptr;
	tec::Simulation simulation;
	tec::GameStateQueue game_state_queue;
	tec::networking::ServerConnection connection;

	VFS.register_extension<tec::MD5Mesh>(".md5mesh", [] (std::string, auto const&, std::iostream& stream, auto& mp_handle, auto next) {
		std::cout << "Real .md5mesh handler" << std::endl;

		try {
			tec::MD5Mesh mesh("", stream);
			mesh.CalculateVertexPositions();
			mesh.CalculateVertexNormals();
			mesh.UpdateIndexList();
			mp_handle.mount(std::move(mesh));
		} catch (std::exception& exc) {
			// Need to pass the resource name to the handler too
			//spdlog::get("console_log")->warn("[MD5Mesh] Error parsing file {}", std::string{real_path});
			spdlog::get("console_log")->warn("[MD5Mesh] Error parsing file {}", exc.what());
			return false;
		}

		std::cout << "Calling next .md5mesh handler" << std::endl;
		if (next.has_value()) {
			next.value()();
		}

		return true;
	});

	VFS.register_extension<tec::MD5Mesh>(".md5mesh", [] (std::string, tec::vfs::virtual_path const& mount_point, std::iostream&, auto&, auto next) {
		std::cout << "DEBUG: Loaded a file at " << std::string{mount_point} << std::endl;

		std::cout << "Calling next .md5mesh handler" << std::endl;
		if (next.has_value()) {
			next.value()();
			return true;
		}
		
		// Not designed to work without an existing loader - doesn't actually load anything!
		return false;
	});

	// Test mounting a directory (will later test loading an archive)
	VFS.mount("/",           tec::fs::path{"./assets"});
	VFS.mount("/assets/",    tec::fs::path{"./assets"});
	VFS.mount("/assets/",    tec::fs::path{"./assets"});

	// Test loading a resource from a directory
	auto meshv1 = VFS.load<tec::MD5Mesh>("/bob/bob");

	// // Test mounting a directory that doesn't exist
	// VFS.mount("/logs/",      tec::fs::path{"./logs"}, tec::vfs::virtual_file_system::create_if_not_exists);

	// // Test creating a resource in a directory
	// auto logfile = VFS.load<tec::LogFile>("/logs/network"/*, create_if_not_exists */);

	// // Test mounting a file as a file
	// VFS.mount("/models/bob", tec::fs::path{"./assets/bob/bob.md5mesh"});

	// // Test loading a file that's already been loaded at a new location
	// auto meshv2 = VFS.load<tec::MD5Mesh>("/models/bob");

	// // Test creating a memory-backed directory
	// VFS.create_ramdisk("/tmp");

	// // Test creating a file in a temporary directory
	// auto tmpfile = VFS.load<tec::TextFile>("/tmp/tmp.ABCDE");

	//return 0;

	console.AddConsoleCommand(
		"msg",
		"msg : Send a message to all clients.",
		[&connection] (const char* args) {
			const char* end_arg = args;
			while (*end_arg != '\0') {
				end_arg++;
			}
			// Args now points were the arguments begins
			std::string message(args, end_arg - args);
			connection.SendChatMessage(message);
		});

	console.AddConsoleCommand(
		"connect",
		"connect ip : Connects to the server at ip",
		[&connection, &log] (const char* args) {
			const char* end_arg = args;
			while (*end_arg != '\0' && *end_arg != ' ') {
				end_arg++;
			}
			// Args now points were the arguments begins
			std::string ip(args, end_arg - args);

			if (!connection.Connect(ip.c_str())) {
				log->error("Failed to connect to " + ip);
			}
		});

	// Get a settings directory path for logging purposes
	log->info(std::string("Loading assets from: ") + tec::FilePath::GetAssetsBasePath().toString());

	log->info("Initializing GUI system...");
	tec::IMGUISystem gui(os.GetWindow());

	gui.CreateGUI(&connection, &console);

	log->info("Initializing rendering system...");
	tec::RenderSystem rs;
	rs.SetViewportSize(os.GetWindowWidth(), os.GetWindowHeight());

	log->info("Initializing simulation system...");
	tec::PhysicsSystem& ps = simulation.GetPhysicsSystem();
	tec::VComputerSystem vcs;

	log->info("Initializing sound system...");
	tec::SoundSystem ss;

	//log->info("Initializing voxel system...");
	//tec::VoxelSystem vox_sys;

	log->info("Initializing script system...");
	tec::LuaSystem lua_sys;

	tec::BuildTestEntities();
	tec::ProtoLoad();

	std::shared_ptr<tec::FPSController> camera_controller;

	double delta = os.GetDeltaTime();
	double mouse_x, mouse_y;

	std::thread ss_thread([&] () { ss.Update(); });

	connection.RegisterMessageHandler(
		tec::networking::MessageType::CLIENT_ID,
		[&game_state_queue, &connection, &camera_controller, &log, &gui] (const tec::networking::ServerMessage&) {
			auto client_id = connection.GetClientID();
			log->info("You are connected as client ID " + std::to_string(client_id));
			camera_controller = std::make_shared<tec::FPSController>(client_id);
			tec::Entity camera(client_id);
			camera.Add<tec::View>(true);
			std::shared_ptr<tec::ControllerAddedEvent> cae_event = std::make_shared<tec::ControllerAddedEvent>();
			cae_event->controller = camera_controller;
			tec::EventSystem<tec::ControllerAddedEvent>::Get()->Emit(cae_event);
			game_state_queue.SetClientID(client_id);

			gui.HideWindow("connect_window");
		});

	connection.RegisterConnectFunc(
		[&connection, &asio_thread, &sync_thread] () {
			asio_thread = new std::thread([&connection] () { connection.StartRead(); });
			sync_thread = new std::thread([&connection] () { connection.StartSync(); });
		});

	double delta_accumulator = 0.0; // Accumulated deltas since the last update was sent.
	tec::state_id_t command_id = 0;

	while (!os.Closing()) {
		os.OSMessageLoop();
		delta = os.GetDeltaTime();
		delta_accumulator += delta;

		ss.SetDelta(delta);
		/*std::async(std::launch::async, [&vox_sys, delta] () {
			vox_sys.Update(delta);
		});*/

		game_state_queue.ProcessEventQueue();
		game_state_queue.Interpolate(delta);

		auto client_state = simulation.Simulate(delta, game_state_queue.GetInterpolatedState());
		if (delta_accumulator >= tec::UPDATE_RATE) {
			if (camera_controller) {
				tec::networking::ServerMessage update_message;
				tec::proto::ClientCommands client_commands = camera_controller->GetClientCommands();
				client_commands.set_commandid(command_id++);
				update_message.SetStateID(connection.GetLastRecvStateID());
				update_message.SetMessageType(tec::networking::MessageType::CLIENT_COMMAND);
				client_commands.SerializeToArray(update_message.GetBodyPTR(), client_commands.ByteSize());
				update_message.SetBodyLength(client_commands.ByteSize());
				update_message.encode_header();
				connection.Send(update_message);
				game_state_queue.SetCommandID(command_id);
			}

			delta_accumulator -= tec::UPDATE_RATE;
		}

		vcs.Update(delta);

		rs.Update(delta, client_state);

		lua_sys.Update(delta);

		//ps.DebugDraw();
		if (camera_controller != nullptr) {
			if (camera_controller->mouse_look) {
				os.EnableMouseLock();
				tec::active_entity = ps.RayCastMousePick(connection.GetClientID(), static_cast<float>(os.GetWindowWidth()) / 2.0f, static_cast<float>(os.GetWindowHeight()) / 2.0f,
														 static_cast<float>(os.GetWindowWidth()), static_cast<float>(os.GetWindowHeight()));
			}
			else {
				os.DisableMouseLock();
				os.GetMousePosition(&mouse_x, &mouse_y);
				tec::active_entity = ps.RayCastMousePick(connection.GetClientID(), mouse_x, mouse_y,
														 static_cast<float>(os.GetWindowWidth()), static_cast<float>(os.GetWindowHeight()));
			}
		}

		gui.Update(delta);
		console.Update(delta);
		os.SwapBuffers();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	ss.Stop();
	ss_thread.join();
	connection.Disconnect();
	connection.Stop();
	if (asio_thread) {
		asio_thread->join();
	}
	if (sync_thread) {
		sync_thread->join();
	}

	return 0;
}
