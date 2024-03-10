import os, platform

tool_prefix = "powerpc-eabi-"

def is_active():
    return True

def get_name():
    return "Nintendo Wii"

def can_build():
    disabled = False

    # Check the minimal dependencies
    devkitpro = os.environ.get("DEVKITPRO", "/opt/devkitpro")
    devkitppc = os.environ.get("DEVKITA64", "/opt/devkitpro/devkitPPC")

    if not os.path.exists(devkitpro):
        print("DevkitPro not found. Wii disabled.")
        disabled = True
    else:
        if not os.path.exists(devkitppc):
            print("DEVKITPPC environment variable is not set correctly.")
            if not os.path.exists("{}/devkitPPC".format(devkitpro)):
                print("DevkitPPC not found. Nintendo Wii disabled.")
                disabled = True
        if not os.path.exists("{}/portlibs/wii/bin/{}pkg-config".format(devkitpro, tool_prefix)):
            print(tool_prefix + "pkg-config not found. Nintendo Wii disabled.")
            disabled = True

    if os.system("pkg-config --version > /dev/null"):
        print("pkg-config not found. Nintendo Wii disabled.")
        disabled = True

    return not disabled

def get_flags():
    return [
        ("tools", False), # Editor is not yet supported on Wii

        # Unsupported on Wii
        ("module_bullet_enabled", False), # Bullet is unsupported due to missing semaphore.h
        ("module_mbedtls_enabled", False), # mbedtls has not been ported to Wii
        ("module_mobile_vr_enabled", False), # Wii is not mobile, nor capable of VR by default
        ("module_theora_enabled", False), # Undefined reference to `oggpack_*`
        ("module_upnp_enabled", False), # UPNP is unsupported due to missing sys/socket.h
        ("module_webm_enabled", False), # WebM is unsupported due to missing semaphore.h
        ("module_websocket_enabled", False), # WebSocket is unsupported due to missing netinet/in.h (wslay)

        # Found in portlibs:
        ("builtin_freetype", False), # ppc-freetype
        ("builtin_libogg", False), # ppc-libogg
        ("builtin_libpng", False), # ppc-libpng
        ("builtin_libvorbis", False), # ppc-libvorbis
        ("builtin_opus", False), # ppc-libopus + ppc-opusfile
        ("builtin_pcre2_with_jit", False), # pcre2 JIT is unsupported
        ("builtin_zlib", False), # ppc-zlib

        # Not found in portlibs, but may be possible
        ("builtin_mbedtls", False), # mbedtls needs to be ported to Wii
    ]

def get_opts():
    from SCons.Variables import EnumVariable
    return [
        EnumVariable("debug_symbols", "Add debugging symbols to release builds", "yes", ("yes", "no", "full")),
    ]

def create(env):
    return env.Clone(tools=["mingw"])

def configure(env):
    # Workaround for MinGW. See:
    # http://www.scons.org/wiki/LongCmdLinesOnWin32
    if os.name == "nt":

        import subprocess

        def mySubProcess(cmdline, env):
            # print("SPAWNED : " + cmdline)
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            proc = subprocess.Popen(
                cmdline,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                startupinfo=startupinfo,
                shell=False,
                env=env,
            )
            data, err = proc.communicate()
            rv = proc.wait()
            if rv:
                print("=====")
                print(err.decode("utf-8"))
                print("=====")
            return rv

        def mySpawn(sh, escape, cmd, args, env):

            newargs = " ".join(args[1:])
            cmdline = cmd + " " + newargs

            rv = 0
            if len(cmdline) > 32000 and cmd.endswith("ar"):
                cmdline = cmd + " " + args[1] + " " + args[2] + " "
                for i in range(3, len(args)):
                    rv = mySubProcess(cmdline + args[i], env)
                    if rv:
                        break
            else:
                rv = mySubProcess(cmdline, env)

            return rv

        env["SPAWN"] = mySpawn

    # Set compilers
    env["CC"] = tool_prefix + "gcc"
    env["CXX"] = tool_prefix + "g++"
    env["LD"] = tool_prefix + "ld"

    dkp = os.environ.get("DEVKITPRO", "/opt/devkitpro")
    dkppc = os.environ.get("DEVKITPPC", "{}/devkitPPC".format(dkp))

    env["ENV"]["DEVKITPRO"] = dkp
    updated_path = "{}/portlibs/wii/bin:{}/bin:".format(dkp, dkppc) + os.environ["PATH"]
    env["ENV"]["PATH"] = updated_path
    os.environ["PATH"] = updated_path  # os environment has to be updated for subprocess calls

    arch = ["-mrvl", "-mcpu=750", "-meabi", "-mhard-float", "-fdata-sections", "-fno-rtti", "-fno-exceptions"]
    env.Prepend(CCFLAGS=arch + ["-ffunction-sections"])

    env.Prepend(CPPPATH=["{}/powerpc-eabi/include".format(dkppc)])
    env.Prepend(CPPFLAGS=["-isystem", "{}/libogc/include".format(dkp)])

    env.Prepend(LIBPATH=["{}/portlibs/ppc/lib".format(dkp), "{}/portlibs/wii/lib".format(dkp), "{}/libogc/lib/wii".format(dkp)])
    env.Prepend(LINKFLAGS=["-mrvl", "-mcpu=750", "-meabi", "-mhard-float", "-T", "platform/wii/pck_embed.ld", "-Wl,--gc-sections"])

    if env["target"] == "release":
        # -O3 -ffast-math is identical to -Ofast. We need to split it out so we can selectively disable
        # -ffast-math in code for which it generates wrong results.
        if env["optimize"] == "speed":  # optimize for speed (default)
            env.Prepend(CCFLAGS=["-O3", "-ffast-math"])
        else:  # optimize for size
            env.Prepend(CCFLAGS=["-Os"])

        if env["debug_symbols"] == "yes":
            env.Prepend(CCFLAGS=["-g1"])
        if env["debug_symbols"] == "full":
            env.Prepend(CCFLAGS=["-g2"])

    elif env["target"] == "release_debug":
        env.Append(CPPDEFINES=["DEBUG_ENABLED"])
        if env["optimize"] == "speed":  # optimize for speed (default)
            env.Prepend(CCFLAGS=["-O2", "-ffast-math"])
        else:  # optimize for size
            env.Prepend(CCFLAGS=["-Os"])

        if env["debug_symbols"] == "yes":
            env.Prepend(CCFLAGS=["-g1"])
        if env["debug_symbols"] == "full":
            env.Prepend(CCFLAGS=["-g2"])

    elif env["target"] == "debug":
        env.Append(CPPDEFINES=["DEBUG_ENABLED", "DEBUG_MEMORY_ENABLED"])
        env.Prepend(CCFLAGS=["-g3"])
        # env.Append(LINKFLAGS=['-rdynamic'])

    ## Architecture

    env["bits"] = "32"

    if env["use_lto"]:
        env.Append(CCFLAGS=["-flto"])
        env.Append(LINKFLAGS=["-flto=" + str(env.GetOption("num_jobs"))])
        env["AR"] = tool_prefix + "gcc-ar"
        env["RANLIB"] = tool_prefix + "gcc-ranlib"
    else:
        env["AR"] = tool_prefix + "ar"
        env["RANLIB"] = tool_prefix + "ranlib"

    # Dependencies

    # freetype depends on libpng and zlib, so bundling one of them while keeping others
    # as shared libraries leads to weird issues
    if env["builtin_freetype"] or env["builtin_libpng"] or env["builtin_zlib"]:
        env["builtin_freetype"] = True
        env["builtin_libpng"] = True
        env["builtin_zlib"] = True

    if not env["builtin_freetype"]:
        env.ParseConfig(tool_prefix + "pkg-config freetype2 --cflags --libs")

    if not env["builtin_libpng"]:
        env.ParseConfig(tool_prefix + "pkg-config libpng --cflags --libs")

    if not env["builtin_zstd"]:
        env.ParseConfig(tool_prefix + "pkg-config libzstd --cflags --libs")

    if env["module_mbedtls_enabled"]:
        env.Append(CPPDEFINES=["MBEDTLS_NO_PLATFORM_ENTROPY"])

    ## Flags

    # Linkflags below this line should typically stay the last ones

    env.Prepend(CPPPATH=["#platform/wii"])
    env.Prepend(CPPDEFINES=[
        "HOMEBREW_ENABLED",
        "WII_ENABLED",
        "GEKKO",
        "NO_THREADS",
        "NO_SAFE_CAST"
        ])
    env.Append(LIBS=["wiiuse", "bte", "fat", "ogc", "m", "ogg", "vorbis", "theora"])
