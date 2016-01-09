## Automated Tests

| Test | Command                       | Input             | Machine | Load          | Compare     | Offset | Timeout |
|:----:|-------------------------------|-------------------|---------|---------------|-------------|:------:|:-------:|
| 1    | ../bin/c2t-96h                | zork.dsk          | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 2    | ../bin/c2t-96h                | dd.po             | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 3    | ../bin/c2t-96h                | zork.dsk          | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 4    | ../bin/c2t-96h                | dd.po             | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 5    | ../bin/c2t-96h -2bcf          | moon.patrol,801   | iie     | LOAD          | mpiie.tiff  | 0      | 25      |
| 6    | ../bin/c2t-96h -2bcf          | moon.patrol,801   | iip     | LOAD          | mpiie.tiff  | 0      | 25      |
| 7    | ../bin/c2t-96h -2acf          | moon.patrol,801   | ii      | 800.A00R 800G | mpii.tiff   | 0      | 25      |
| 8    | ../bin/c2t-96h -2bc8          | super_puckman,800 | iie     | LOAD          | spiie.tiff  | 0      | 25      |
| 9    | ../bin/c2t-96h -2bc8          | super_puckman,800 | iip     | LOAD          | spiie.tiff  | 0      | 25      |
| 10   | ../bin/c2t-96h -2ac8          | super_puckman,800 | ii      | 800.A00R 800G | spiie.tiff  | 0      | 25      |
| 11   | wine ../bin/c2t-96h.exe       | zork.dsk          | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 12   | wine ../bin/c2t-96h.exe       | dd.po             | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 13   | wine ../bin/c2t-96h.exe       | zork.dsk          | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 14   | wine ../bin/c2t-96h.exe       | dd.po             | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 15   | wine ../bin/c2t-96h.exe -2bcf | moon.patrol,801   | iie     | LOAD          | mpiie.tiff  | 0      | 25      |
| 16   | wine ../bin/c2t-96h.exe -2bcf | moon.patrol,801   | iip     | LOAD          | mpiie.tiff  | 0      | 25      |
| 17   | wine ../bin/c2t-96h.exe -2acf | moon.patrol,801   | ii      | 800.A00R 800G | mpii.tiff   | 0      | 25      |
| 18   | wine ../bin/c2t-96h.exe -2bc8 | super_puckman,800 | iie     | LOAD          | spiie.tiff  | 0      | 25      |
| 19   | wine ../bin/c2t-96h.exe -2bc8 | super_puckman,800 | iip     | LOAD          | spiie.tiff  | 0      | 25      |
| 20   | wine ../bin/c2t-96h.exe -2ac8 | super_puckman,800 | ii      | 800.A00R 800G | spiie.tiff  | 0      | 25      |

### Future Edit Instructions Here
