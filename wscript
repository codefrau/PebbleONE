
#
# build customized to embed HTML in Javascript
#

top = '.'
out = 'build'

def options(ctx):
    ctx.load('pebble_sdk')

def configure(ctx):
    ctx.load('pebble_sdk')

def build(ctx):
    ctx.load('pebble_sdk')

    ctx.pbl_program(source=ctx.path.ant_glob('src/**/*.c'),
                    target='pebble-app.elf')

    # generate config.js from config.html by escaping every line and quotes
    config_html = ctx.path.make_node('src/js/config.html')
    config_js  = ctx.path.get_bld().make_node('src/js/config.js')
    ctx(rule='(echo config_html= && sed "s/\'/\\\\\\\'/g;s/^/\\\'/;s/$/\\\' +/" ${SRC} && echo "\'\'") > ${TGT}', source=config_html, target=config_js)

    # append config.js to pebble-js-app.js
    src_js   = ctx.path.make_node('src/js/pebble-js-app.js')
    build_js  = ctx.path.get_bld().make_node('src/js/pebble-js-app.js')
    ctx(rule='cat ${SRC} > ${TGT}', source=[src_js, config_js], target=build_js)

    # use build/src/js/pebble-js-app.js
    ctx.pbl_bundle(elf='pebble-app.elf', js=build_js)
