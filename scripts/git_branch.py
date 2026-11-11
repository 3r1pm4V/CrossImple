import os
import subprocess
import re

try:
    Import('env')
except NameError:
    # Not running within PlatformIO build environment (e.g. CLI test)
    env = None


def get_git_branch(project_dir):
    try:
        # Check if HEAD is detached (GitHub Actions)
        branch_output = subprocess.check_output(
            ['git', 'branch', '--show-current'],
            cwd=project_dir,
            stderr=subprocess.DEVNULL,
        ).decode('utf-8').strip()

        if branch_output:
            return branch_output

        # If detached HEAD, try to get branch from GitHub Actions environment variable
        github_ref = os.environ.get('GITHUB_REF')
        if github_ref and github_ref.startswith('refs/heads/'):
            return github_ref.replace('refs/heads/', '')

        # Fallback: get commit hash
        commit_output = subprocess.check_output(
            ['git', 'rev-parse', '--short', 'HEAD'],
            cwd=project_dir,
            stderr=subprocess.DEVNULL,
        ).decode('utf-8').strip()

        return commit_output if commit_output else 'unknown'
    except Exception:
        return 'unknown'


def get_crossink_version(project_dir):
    app_version_h = os.path.join(project_dir, 'lib', 'AppVersion', 'AppVersion.h')
    try:
        with open(app_version_h, 'r') as f:
            for line in f:
                if '#define APP_VERSION' in line:
                    match = re.search(r'#define APP_VERSION\s+"([^"]+)"', line)
                    if match:
                        return match.group(1)
        return '0.24'
    except Exception:
        return '0.24'


def get_production_version(project_dir):
    return '0.24'


def get_hardware_version(project_dir, pioenv):
    if pioenv == 'default':
        return 'v0.24'
    if pioenv == 'x4-classic':
        return 'v0.24-x4-classic'
    if pioenv == 'x4-pro':
        return 'v0.24-x4-pro'
    return 'v0.24'


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
        ('CROSSINK_VERSION', '\\\"' + hardware_version + '\\\"'),
    ])

    inject_download_endpoints(env)


if env:
    inject_version(env)
