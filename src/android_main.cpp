
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <SDL3/SDL.h>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_sdl.h>

// SDL3 Android loads libmain.so and invokes SDL_main().
extern "C" SDL_DECLSPEC int SDLCALL SDL_main(int argc, char* argv[]) {
#if defined(__ANDROID__)
  // Configure app-private storage before ReXApp initializes its paths.
  char* preference_path = SDL_GetPrefPath("CentralGamer", "MCLA");

  if (preference_path && *preference_path) {
    const std::filesystem::path app_path(preference_path);

    // ReXGlue appends the application name to XDG_DATA_HOME.
    // Use the parent so it does not append MCLA twice.
    const std::string xdg_data_home =
        app_path.parent_path().string();

    if (!xdg_data_home.empty() &&
        setenv("XDG_DATA_HOME", xdg_data_home.c_str(), 1) != 0) {
      SDL_Log("Could not set XDG_DATA_HOME");
    }
  } else {
    SDL_Log("SDL_GetPrefPath failed: %s", SDL_GetError());
  }

  SDL_free(preference_path);
#endif

  auto remaining = rex::cvar::Init(argc, argv);
  rex::cvar::ApplyEnvironment();
  rex::InitLoggingEarly();

  int result = EXIT_FAILURE;

  {
    rex::ui::SDLWindowedAppContext context;

    if (!context.Initialize()) {
      REXLOG_ERROR("Failed to initialize SDL3 UI context.");
      return EXIT_FAILURE;
    }

    auto creator = rex::ui::WindowedApp::GetCreator("mcla");

    if (!creator) {
      REXLOG_ERROR("MCLA app creator was not registered.");
      return EXIT_FAILURE;
    }

    std::unique_ptr<rex::ui::WindowedApp> app = creator(context);

    const auto& option_names = app->GetPositionalOptions();
    std::map<std::string, std::string> parsed;

    const size_t count =
        std::min(remaining.size(), option_names.size());

    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }

    app->SetParsedArguments(std::move(parsed));

    if (app->OnInitialize()) {
      result = context.RunMainMessageLoop();
    } else {
      REXLOG_ERROR("MCLA ReXApp initialization failed.");
    }

    app->InvokeOnDestroy();
  }

  return result;
}
