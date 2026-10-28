
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_sdl.h>

#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace {

void MclaBootLog(const std::string& message) {
#if defined(__ANDROID__)
  __android_log_write(ANDROID_LOG_INFO, "MCLA_BOOT", message.c_str());

  char* preference_path = SDL_GetPrefPath("CentralGamer", "MCLA");
  if (preference_path && *preference_path) {
    std::error_code error;
    const std::filesystem::path directory(preference_path);
    std::filesystem::create_directories(directory, error);

    if (!error) {
      std::ofstream log(directory / "startup-trace.log",
                        std::ios::app);
      if (log.is_open()) {
        log << message << '\n';
        log.flush();
      }
    }
  }
  SDL_free(preference_path);
#else
  SDL_Log("MCLA_BOOT: %s", message.c_str());
#endif
}

}  // namespace

// SDL3 Android loads libmain.so and invokes SDL_main().
extern "C" SDL_DECLSPEC int SDLCALL SDL_main(int argc, char* argv[]) {
  MclaBootLog("01 - SDL_main entrou");

#if defined(__ANDROID__)
  char* preference_path = SDL_GetPrefPath("CentralGamer", "MCLA");

  if (preference_path && *preference_path) {
    const std::filesystem::path app_path(preference_path);
    const std::string xdg_data_home =
        app_path.parent_path().string();

    if (!xdg_data_home.empty() &&
        setenv("XDG_DATA_HOME", xdg_data_home.c_str(), 1) != 0) {
      MclaBootLog("ERRO - nao foi possivel definir XDG_DATA_HOME");
    } else {
      MclaBootLog("02 - armazenamento privado preparado");
    }
  } else {
    MclaBootLog(std::string("ERRO - SDL_GetPrefPath: ") +
                SDL_GetError());
  }

  SDL_free(preference_path);
#endif

  MclaBootLog("03 - inicializando cvars e logging");

  auto remaining = rex::cvar::Init(argc, argv);
  rex::cvar::ApplyEnvironment();
  rex::InitLoggingEarly();

  int result = EXIT_FAILURE;

  {
    MclaBootLog("04 - criando contexto SDLWindowedAppContext");
    rex::ui::SDLWindowedAppContext context;

    if (!context.Initialize()) {
      MclaBootLog(std::string("FALHA - contexto SDL3: ") +
                  SDL_GetError());
      REXLOG_ERROR("Failed to initialize SDL3 UI context.");
      return EXIT_FAILURE;
    }

    MclaBootLog("05 - contexto SDL3 inicializado");

    auto creator = rex::ui::WindowedApp::GetCreator("mcla");

    if (!creator) {
      MclaBootLog("FALHA - criador MCLA nao registrado");
      REXLOG_ERROR("MCLA app creator was not registered.");
      return EXIT_FAILURE;
    }

    MclaBootLog("06 - criador MCLA encontrado");

    std::unique_ptr<rex::ui::WindowedApp> app = creator(context);

    if (!app) {
      MclaBootLog("FALHA - criador retornou app nulo");
      return EXIT_FAILURE;
    }

    MclaBootLog("07 - instancia MCLA criada");

    const auto& option_names = app->GetPositionalOptions();
    std::map<std::string, std::string> parsed;

    const size_t count =
        std::min(remaining.size(), option_names.size());

    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }

    app->SetParsedArguments(std::move(parsed));

    MclaBootLog("08 - iniciando OnInitialize");
    const bool initialized = app->OnInitialize();

    MclaBootLog(initialized
                    ? "09 - OnInitialize retornou true"
                    : "FALHA - OnInitialize retornou false");

    if (initialized) {
      MclaBootLog("10 - entrando no loop de eventos");
      result = context.RunMainMessageLoop();
      MclaBootLog("11 - loop de eventos terminou");
    } else {
      REXLOG_ERROR("MCLA ReXApp initialization failed.");
    }

    MclaBootLog("12 - chamando InvokeOnDestroy");
    app->InvokeOnDestroy();
  }

  MclaBootLog("13 - SDL_main terminou");
  return result;
}
