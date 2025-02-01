import pathlib, subprocess, os, shutil, tomllib
import build_n64recomp_tools as bnt

mod_data = tomllib.loads(pathlib.Path("mod.toml").read_text())
mod_manifest_data = mod_data["manifest"]
# print(mod_data)
build_dir = pathlib.Path(__file__).parent.joinpath(f"build")
build_nrm_file = build_dir.joinpath(f"{mod_data['inputs']['mod_filename']}.nrm")
build_dll_file = build_nrm_file.with_suffix(".dll")
build_pdb_file = build_nrm_file.with_suffix(".pdb")

runtime_mods_dir = pathlib.Path(__file__).parent.joinpath("runtime/mods")
runtime_nrm_file = runtime_mods_dir.joinpath(f"{mod_data['inputs']['mod_filename']}.nrm")
runtime_dll_file = runtime_nrm_file.with_suffix(".dll")
runtime_pdb_file = runtime_nrm_file.with_suffix(".pdb")


if not bnt.build_dir.exists():
    print("N64Recomp tools not built. Building now...")
    bnt.rebuild_tools();

deps = bnt.get_dependencies()

make_run = subprocess.run(deps["make"])
if make_run.returncode != 0:
    raise RuntimeError("Make failed to build mod binaries.")

RecompModTool_run = subprocess.run(
    [
        bnt.get_RecompModTool_path(),
        "mod.toml",
        "build"
    ],
    cwd=os.getcwd()
)
if RecompModTool_run.returncode != 0:
    raise RuntimeError("RecompModTool failed to build mod.")

OfflineModRecomp_run = subprocess.run(
    [
        bnt.get_OfflineModRecomp_path(),
        "build/mod_syms.bin",
        "build/mod_binary.bin",
        "Zelda64RecompSyms/mm.us.rev1.syms.toml",
        "build/mod_recompiled.c",
    ],
    cwd=os.getcwd()
)
if OfflineModRecomp_run.returncode != 0:
    raise RuntimeError("OfflineModRecomp failed to generate 'mod_recompiled.c'.")

# Compile DLL:
compiler_run = subprocess.run(
    [
        deps["clang-cl"], 
        "build/mod_recompiled.c", 
        "-fuse-ld=lld", 
        "-Z7",
        "/Ioffline_build",
        "/Imm-decomp/include",
        "/Imm-decomp/overlays/gamestates/ovl_file_choose/",
        "/MD",
        "/O2",
        "/link",
        "/DLL",
        f"/OUT:{build_dll_file}"
    ]
)
if compiler_run.returncode != 0:
    raise RuntimeError("'mod_recompiled.c' could not be compiled.")


# Copying files for debugging:
os.makedirs(runtime_mods_dir, exist_ok=True)
shutil.copy(build_nrm_file, runtime_nrm_file)
shutil.copy(build_dll_file, runtime_dll_file)
shutil.copy(build_pdb_file, runtime_pdb_file)
