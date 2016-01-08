## Automated Tests


| Test | Command                  | Input           | Machine | Load          | Compare     | Offset | Timeout |
|:----:|--------------------------|-----------------|---------|---------------|-------------|:------:|:-------:|
| 1    | ../c2t-96h               | zork.dsk        | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 2    | wine ../c2t-96h.exe      | zork.dsk        | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 3    | ../c2t-96h               | dd.po           | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 4    | wine ../c2t-96h.exe      | dd.po           | iie     | LOAD          | dskiie.tiff | 0      | 25      |
| 5    | ../c2t-96h               | zork.dsk        | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 6    | wine ../c2t-96h.exe      | zork.dsk        | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 7    | ../c2t-96h               | dd.po           | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 8    | wine ../c2t-96h.exe      | dd.po           | iip     | LOAD          | dskiip.tiff | 0      | 25      |
| 9    | ../c2t-96h -2bf          | moon.patrol,801 | iie     | LOAD          | mpiie.tiff  | 0      | 25      |
| 10   | wine ../c2t-96h.exe -2bf | moon.patrol,801 | iie     | LOAD          | mpiie.tiff  | 0      | 25      |
| 11   | ../c2t-96h -2af          | moon.patrol,801 | ii      | 800.A00R 800G | mpii.tiff   | 0      | 25      |
| 12   | wine ../c2t-96h.exe -2af | moon.patrol,801 | ii      | 800.A00R 800G | mpii.tiff   | 0      | 25      |
