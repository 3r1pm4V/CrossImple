import configparser
import os
import re
import subprocess
import sys

try:
    Import('env')
except NameError:
    # Not running within PlatformIO build environment (e.g. CLI test)
    env = None


def warn(msg):
    print(f'WARNING [git_branch.py]: {msg}', file=sys.stderr)


def run_git_value(project_dir, args, label):
    try:
        value = subprocess.check_output(
            ['git', *args],
            text=True, stderr=subprocess.PIPE, cwd=project_dir
        ).strip()
        return ''.join(c for c in value if c not in '"\\')
    except FileNotFoundError:
        warn(f'git not found on PATH; {label} suffix will be "unknown"')
        return 'unknown'
    except subprocess.CalledProcessError as e:
        warn(
            f'git command failed (exit {e.returncode}): '
            f'{e.stderr.strip()}; {label} suffix will be "unknown"'
        )
        return 'unknown'
    except OSError as e:
        warn(
            f'OS error reading git {label}: {e}; '
            f'{label} suffix will be "unknown"'
        )
        return 'unknown'
    except Exception as e:
        warn(
            f'Unexpected error reading git {label}: {e}; '
            f'{label} suffix will be "unknown"'
        )
        return 'unknown'


def sanitize_version_component(value):
    value = value.strip()
    value = re.sub(r'[^A-Za-z0-9._-]+', '-', value)
    value = re.sub(r'-{2,}', '-', value)
    value = value.strip('-.')
    return value or 'unknown'


def get_git_branch(project_dir):
    try:
        branch_output = subprocess.check_output(
            ['git', 'branch', '--show-current'],
            cwd=project_dir,
            stderr=subprocess.DEVNULL,
        ).decode('utf-8').strip()

        if branch_output:
            return sanitize_version_component(branch_output)

        github_ref = os.environ.get('GITHUB_REF')
        if github_ref and github_ref.startswith('refs/heads/'):
            return sanitize_version_component(github_ref.replace('refs/heads/', ''))

        commit_output = subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD'],
            cwd=project_dir,
            stderr=subprocess.DEVNULL,
        ).decode('utf-8').strip()

        return commit_output if commit_output else 'unknown'
    except Exception:
        return 'unknown'


def get_git_short_sha(project_dir):
    return run_git_value(
        project_dir, ['rev-parse', '--short', 'HEAD'], 'short SHA'
    )


def get_git_short_hash(project_dir, length=5):
    try:
        return subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD'],
            text=True, stderr=subprocess.PIPE, cwd=project_dir
        ).strip()[:length]
    except Exception as e:
        warn(f'Could not read git hash: {e}; hash will be "00000"')
        return '00000'


def get_git_dirty(project_dir):
    try:
        status = subprocess.check_output(
            ['git', 'status', '--porcelain', '--untracked-files=no'],
            text=True, stderr=subprocess.PIPE, cwd=project_dir
        )
        return '1' if status.strip() else '0'
    except Exception as e:
        warn(f'Could not read git working-tree state: {e}; state will be "unknown"')
        return 'unknown'


def _read_crossink_version_from_ini(project_dir):
    ini_path = os.path.join(project_dir, 'platformio.ini')
    local_ini_path = os.path.join(project_dir, 'platformio.local.ini')
    config = configparser.ConfigParser()
    if os.path.isfile(ini_path):
        config_paths = [ini_path]
        if os.path.isfile(local_ini_path):
            config_paths.append(local_ini_path)
        config.read(config_paths)
        if config.has_option('crossink', 'version'):
            return config.get('crossink', 'version').strip()
    return None


def get_crossink_version(env_or_dir):
    if hasattr(env_or_dir, 'GetProjectOption'):
        try:
            val = env_or_dir.GetProjectOption('crossink.version')
            if val:
                return str(val).strip()
        except Exception:
            pass
    if hasattr(env_or_dir, 'GetProjectConfig'):
        try:
            val = env_or_dir.GetProjectConfig().get('crossink', 'version')
            if val:
                return str(val).strip()
        except Exception:
            pass
    project_dir = env_or_dir if isinstance(env_or_dir, str) else (
        getattr(env_or_dir, 'get', lambda k, d=None: d)('PROJECT_DIR', os.getcwd())
        if hasattr(env_or_dir, 'get') else os.getcwd()
    )
    val = _read_crossink_version_from_ini(project_dir)
    if val:
        return val
    return '0.0.0'


def get_production_version(project_dir):
    release_version = os.environ.get('CROSSINK_RELEASE_VERSION')
    if release_version:
        return sanitize_version_component(release_version.lstrip('v'))
    return get_crossink_version(project_dir)


def get_hardware_version(project_dir, pioenv):
    ver = get_crossink_version(project_dir)
    base = f'v{ver}' if not ver.startswith('v') else ver
    if pioenv in ('default', 'debug', ''):
        return base
    if pioenv.startswith('x4-classic'):
        return f'{base}-x4-classic'
    if pioenv.startswith('x4-pro'):
        return f'{base}-x4-pro'
    if pioenv.startswith('metalio-eink4'):
        return f'{base}-metalio-eink4'
    return f'{base}-{pioenv}'


def inject_download_endpoints(env):
    # Explicit build configuration only; never read an endpoint from SD/catalogs.
    for variable, define, scheme in (
        ('CROSSINK_ASSET_BASE_URL', 'ASSET_DOWNLOAD_BASE_URL', 'https'),
        ('CROSSINK_FONT_BASE_URL', 'FONT_DOWNLOAD_BASE_URL', 'https?'),
    ):
        base = os.environ.get(variable, '')
        if not base:
            continue
        if not re.fullmatch(scheme + r'://[A-Za-z0-9.-]+(?::[0-9]+)?/(?:[A-Za-z0-9_-]+/)*', base):
            protocols = 'HTTP(S)' if scheme == 'https?' else 'HTTPS'
            raise ValueError(f'{variable} must be an {protocols} prefix ending in /')
        env.Append(CPPDEFINES=[(define, '\"' + base + '\"')])


def inject_version(env):
    project_dir = env.get('PROJECT_DIR', os.getcwd())
    pioenv = env.get('PIOENV', '')

    branch = get_git_branch(project_dir)
    hardware_version = get_hardware_version(project_dir, pioenv)

    env.Append(CPPDEFINES=[
        ('GIT_BRANCH', '\\\"' + branch + '\\\"'),
        ('CROSSINK_HARDWARE_VERSION', '\\\"' + hardware_version + '\\\"'),
    ])

    inject_download_endpoints(env)

    canonical_version = (
        get_production_version(project_dir)
        if os.environ.get('CROSSINK_RELEASE_VERSION') and not os.environ.get('CROSSINK_RC_HASH')
        else get_crossink_version(project_dir)
    )
    if not re.fullmatch(r"[0-9]+\.[0-9]+(?:\.[0-9]+)?", canonical_version):
        canonical_version = "unknown"
    sdk_sha = run_git_value(os.path.join(project_dir, "freeink-sdk"), ["rev-parse", "HEAD"], "SDK SHA")
    if not re.fullmatch(r"[0-9a-f]{40}", sdk_sha):
        sdk_sha = "unknown"

    scoped = [
        ('CROSSINK_SUPPORT_VERSION', f'\\"{canonical_version}\\"'),
        ('CROSSINK_SDK_SHA', f'\\"{sdk_sha}\\"'),
        ('CROSSINK_GIT_SHA', f'\\"{get_git_short_sha(project_dir)}\\"'),
        ('CROSSINK_GIT_DIRTY', f'\\"{get_git_dirty(project_dir)}\\"'),
    ]

    if hasattr(env, 'AddBuildMiddleware'):
        def add_build_info_defines(node_env, node):
            node_str = str(node).replace('\\', '/')
            if not node_str.endswith('BuildInfo.cpp'):
                return node
            if getattr(node_env.Object, '__name__', '') == 'custom_object_wrapper':
                combined = list(node_env.get('CPPDEFINES', [])) + scoped
                return node_env.Object(node, CPPDEFINES=combined)
            build_env = node_env.Clone()
            build_env.Append(CPPDEFINES=scoped)
            return build_env.Object(node)
        env.AddBuildMiddleware(add_build_info_defines, '*src/util/BuildInfo.cpp')
    else:
        env.Append(CPPDEFINES=scoped)


if env:
    inject_version(env)
