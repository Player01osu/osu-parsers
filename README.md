
# osu! Parsers

Relatively self-contained parsers for osu! file formats written in c99.

## Usage

Create static libraries with:

```console
make static
```

`lib*_parser.a` is created in the project directory

You might also want to copy the header files or add this to your include paths

An example of building an executable is in the `Makefile`

## Resources

- https://github.com/ppy/osu
- https://github.com/dotnet/runtime
- https://github.com/lloyd/easylzma
- https://github.com/nothings/stb/blob/master/stb_sprintf.h

## TODO

- [ ] Shared library
- [ ] Document API
- [ ] Parse .osu
- [ ] Prefix symbols
