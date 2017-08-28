# encoding=utf_8

import os
import xlrd
import json

def trans_xls2json(filename):	
	# 读取表头和表内容
	filepath = os.path.normpath(os.path.join(os.getcwd(), "Excel/%s"%(filename)))
	book = xlrd.open_workbook(filepath)
	sheet = book.sheet_by_index(0)
	titles = [x.encode("utf_8") for x in sheet.row_values(0)]
	rows = [sheet.row_values(x) for x in xrange(1, sheet.nrows)]

	# 将每一行转化为字典，存入列表
	cell_list = []
	for x in rows:
		cell = {}
		for col in xrange(0,len(titles)):
			if type(x[col])==unicode:
				cell[titles[col]] = x[col].encode("utf_8")
			elif type(x[col])==float and x[col]%1 == 0.0:
				cell[titles[col]] = int(x[col])
			else:
				cell[titles[col]] = x[col]
		cell_list.append(cell)

	# 编码为json格式，输出
	outputpath = os.path.normpath(os.path.join(os.getcwd(), "config/%s.json"%(os.path.splitext(filename)[0]) ))
	f = open(outputpath, "wb")
	# result = {"depu_language" : cell_list}
	json.dump(cell_list, f,  sort_keys=True, indent=4, encoding='utf-8', ensure_ascii=False)
	f.close()

xls_list = [x for x in os.listdir("./Excel/") if os.path.splitext(x)[1]=='.xls']
for x in xls_list:
	trans_xls2json(x)

# l1 = [x for x in range(1, 10)]
# print(l1)
# l2 = map(lambda x: x*2, l1)
# print(l2)
# print(help(map))
# reduce(lambda x: print(x), l1)
