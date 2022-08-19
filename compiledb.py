Import("env")
env.Tool('compilation_db')
env.CompilationDatabase('compile_commands.json')