# make file for ldbrock
#

from os.path import join as dirjoin

def_env = DefaultEnvironment()
MakeBuilder = Builder(action = 'make -j %d ${TARGET.file}' % GetOption("num_jobs"), target_factory=Dir, chdir=1)
def_env.Append(BUILDERS = {"Make" : MakeBuilder})


targets = {}

SUB_DIRS = {
        'linenoise': '3rd-party/linenoise',
        'ldbrock': 'src',
}

leveldb_mk = def_env.Make('leveldb/all', None)
targets['leveldb'] = leveldb_mk

for mname, sd in SUB_DIRS.iteritems():
    targets[mname] = SConscript(dirjoin(sd, 'SConscript'), variant_dir = dirjoin('build', sd))

Depends(targets['ldbrock'], [targets['leveldb'], targets['linenoise']])

# "run" PHONY target
Alias('run', None, './build/src/ldbrock')
AlwaysBuild('run')


Default(targets['ldbrock'])

