# suduku_count
Count suduku candidate percentages

// Suduku_count
// Attempt to count the number of legal suduku positions
// Based on Discussion between Kendrick Shaw MD PhD and Paul Alfille MD
// MIT license 2019
// by Paul H Alfille

Basically creates candidate squares (row by row unless no value possible)
Then tests 3x3 subsquares

Example output:
[paul@radiohead suduku_count]$ ./sudoku_count 
Bad=1503505, Candidate=10000, Good=0
		0.660718%	0.000000%
Bad=3051824, Candidate=20000, Good=0
		0.651079%	0.000000%
Bad=4571444, Candidate=30000, Good=0
		0.651969%	0.000000%
Bad=6125560, Candidate=40000, Good=0
		0.648765%	0.000000%
Bad=7674037, Candidate=50000, Good=0
		0.647330%	0.000000%
Bad=9195857, Candidate=60000, Good=0
		0.648238%	0.000000%
Bad=10739132, Candidate=70000, Good=0
		0.647601%	0.000000%
Bad=12293860, Candidate=80000, Good=0
		0.646524%	0.000000%

+---+---+---+---+---+---+---+---+---+
| 4 | 7 | 1 | 2 | 8 | 5 | 3 | 6 | 9 |
+---+---+---+---+---+---+---+---+---+
| 9 | 5 | 6 | 1 | 3 | 7 | 8 | 4 | 2 |
+---+---+---+---+---+---+---+---+---+
| 3 | 8 | 2 | 4 | 6 | 9 | 7 | 1 | 5 |
+---+---+---+---+---+---+---+---+---+
| 8 | 2 | 9 | 6 | 7 | 4 | 5 | 3 | 1 |
+---+---+---+---+---+---+---+---+---+
| 7 | 3 | 5 | 8 | 2 | 1 | 6 | 9 | 4 |
+---+---+---+---+---+---+---+---+---+
| 1 | 6 | 4 | 9 | 5 | 3 | 2 | 8 | 7 |
+---+---+---+---+---+---+---+---+---+
| 5 | 4 | 7 | 3 | 9 | 8 | 1 | 2 | 6 |
+---+---+---+---+---+---+---+---+---+
| 2 | 1 | 8 | 7 | 4 | 6 | 9 | 5 | 3 |
+---+---+---+---+---+---+---+---+---+
| 6 | 9 | 3 | 5 | 1 | 2 | 4 | 7 | 8 |
+---+---+---+---+---+---+---+---+---+

Bad=13503275, Candidate=87913, Good=1
		0.646838%	0.000007%
Bad=13843204, Candidate=90000, Good=1
		0.645939%	0.000007%
Bad=15368091, Candidate=100000, Good=1
		0.646492%	0.000006%
Bad=16865398, Candidate=110000, Good=1
		0.647997%	0.000006%
Bad=18376847, Candidate=120000, Good=1
		0.648759%	0.000005%
Bad=19909239, Candidate=130000, Good=1
		0.648727%	0.000005%
Bad=21450058, Candidate=140000, Good=1
		0.648447%	0.000005%
Bad=22972474, Candidate=150000, Good=1
		0.648719%	0.000004%
Bad=24482452, Candidate=160000, Good=1
		0.649286%	0.000004%
Bad=26014815, Candidate=170000, Good=1
		0.649231%	0.000004%
Bad=27540147, Candidate=180000, Good=1
		0.649347%	0.000004%
Bad=29078012, Candidate=190000, Good=1
		0.649173%	0.000003%
Bad=30598587, Candidate=200000, Good=1
		0.649380%	0.000003%
Bad=32140638, Candidate=210000, Good=1
		0.649137%	0.000003%
Bad=33686990, Candidate=220000, Good=1
		0.648834%	0.000003%
Bad=35224212, Candidate=230000, Good=1
		0.648724%	0.000003%
Bad=36750937, Candidate=240000, Good=1
		0.648808%	0.000003%
Bad=38271884, Candidate=250000, Good=1
		0.648982%	0.000003%
Bad=39793300, Candidate=260000, Good=1
		0.649135%	0.000002%
Bad=41323530, Candidate=270000, Good=1
		0.649139%	0.000002%
Bad=42854138, Candidate=280000, Good=1
		0.649138%	0.000002%
Bad=44394492, Candidate=290000, Good=1
		0.648995%	0.000002%
Bad=45912247, Candidate=300000, Good=1
		0.649179%	0.000002%
Bad=47431986, Candidate=310000, Good=1
		0.649324%	0.000002%
Bad=48939978, Candidate=320000, Good=1
		0.649615%	0.000002%
Bad=50493590, Candidate=330000, Good=1
		0.649305%	0.000002%
Bad=52035099, Candidate=340000, Good=1
		0.649163%	0.000002%
Bad=53545060, Candidate=350000, Good=1
		0.649410%	0.000002%
Bad=55075213, Candidate=360000, Good=1
		0.649407%	0.000002%
Bad=56615600, Candidate=370000, Good=1
		0.649287%	0.000002%
Bad=58158175, Candidate=380000, Good=1
		0.649149%	0.000002%
Bad=59665228, Candidate=390000, Good=1
		0.649402%	0.000002%
Bad=61187580, Candidate=400000, Good=1
		0.649482%	0.000002%
Bad=62698343, Candidate=410000, Good=1
		0.649676%	0.000002%
Bad=64283116, Candidate=420000, Good=1
		0.649119%	0.000002%
Bad=65811961, Candidate=430000, Good=1
		0.649135%	0.000002%
Bad=67320982, Candidate=440000, Good=1
		0.649341%	0.000001%
Bad=68837583, Candidate=450000, Good=1
		0.649467%	0.000001%
Bad=70377768, Candidate=460000, Good=1
		0.649371%	0.000001%
Bad=71891688, Candidate=470000, Good=1
		0.649515%	0.000001%
Bad=73457711, Candidate=480000, Good=1
		0.649195%	0.000001%
Bad=74968945, Candidate=490000, Good=1
		0.649360%	0.000001%
Bad=76521355, Candidate=500000, Good=1
		0.649171%	0.000001%
Bad=78060636, Candidate=510000, Good=1
		0.649097%	0.000001%
Bad=79632125, Candidate=520000, Good=1
		0.648766%	0.000001%
Bad=81166156, Candidate=530000, Good=1
		0.648745%	0.000001%
Bad=82709978, Candidate=540000, Good=1
		0.648649%	0.000001%
Bad=84211242, Candidate=550000, Good=1
		0.648881%	0.000001%
Bad=85730827, Candidate=560000, Good=1
		0.648968%	0.000001%
Bad=87282667, Candidate=570000, Good=1
		0.648814%	0.000001%
Bad=88808409, Candidate=580000, Good=1
		0.648854%	0.000001%
Bad=90358498, Candidate=590000, Good=1
		0.648719%	0.000001%
Bad=91876301, Candidate=600000, Good=1
		0.648815%	0.000001%
Bad=93430841, Candidate=610000, Good=1
		0.648654%	0.000001%
Bad=94970904, Candidate=620000, Good=1
		0.648597%	0.000001%
Bad=96505735, Candidate=630000, Good=1
		0.648577%	0.000001%
Bad=98055708, Candidate=640000, Good=1
		0.648458%	0.000001%
Bad=99351528, Candidate=648471, Good=1
		0.65%	0.00%
