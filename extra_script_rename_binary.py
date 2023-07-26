Import("env")
import os

env.Replace(PROGNAME="firmware_{dir}_v{version}".format(
    dir=os.path.basename(os.getcwd()), 
    version=env.GetProjectOption("custom_prog_version")
))
# env.Replace(PROGNAME="firmware_%s" % env.GetProjectOption("custom_prog_version"))