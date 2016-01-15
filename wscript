
#
# build customized to embed HTML in Javascript
#

import os.path

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')

    # generate config.js from config.html by escaping every line and quotes
    config_html = ctx.path.make_node('src/config.html')
    config_js  = ctx.path.get_bld().make_node('src/js/config.js')
    ctx(rule='(echo config_html= && sed "s/\'/\\\\\\\'/g;s/^/\\\'/;s/$/\\\' +/" ${SRC} && echo "\'\';") > ${TGT}', source=config_html, target=config_js)

    # make pebble-js-app.js by appending config.js to pebble_one.js
    # and run jshint on the result
    src_js   = ctx.path.make_node('src/pebble_one.js')
    build_js  = ctx.path.get_bld().make_node('src/js/pebble-js-app.js')
    ctx(rule='(cat ${SRC} > ${TGT} && jshint --config ../pebble-jshintrc ${TGT})', source=[src_js, config_js], target=build_js)

    # build binaries for each platform
    build_worker = os.path.exists('worker_src')
    binaries = []

    for p in ctx.env.TARGET_PLATFORMS:
        ctx.set_env(ctx.all_envs[p])
        ctx.set_group(ctx.env.PLATFORM_NAME)
        app_elf='{}/pebble-app.elf'.format(ctx.env.BUILD_DIR)
        ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
        target=app_elf)

        if build_worker:
            worker_elf='{}/pebble-worker.elf'.format(ctx.env.BUILD_DIR)
            binaries.append({'platform': p, 'app_elf': app_elf, 'worker_elf': worker_elf})
            ctx.pbl_worker(source=ctx.path.ant_glob('worker_src/**/*.c'),
            target=worker_elf)
        else:
            binaries.append({'platform': p, 'app_elf': app_elf})

    ctx.set_group('bundle')
    ctx.pbl_bundle(binaries=binaries, js=build_js)
