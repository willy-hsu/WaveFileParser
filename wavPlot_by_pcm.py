import array
import os
from matplotlib import pyplot

# plot pcm waveform by python reference : https://blog.csdn.net/dennyli1986/article/details/101233132
# python array type reference : https://docs.python.org/zh-tw/3.9/library/array.html
# dataName = "divertimento_192k_2ch_24b_short_FRONT_LEFT"
dataName = "beemoved_96k_2ch_16b_short_FRONT_LEFT"
pcmData1 = array.array('i')
pcmData2 = array.array('i')
pcmData3 = array.array('i')

fileName = "output/LOST_none_COMP_none/pcm/MY_{}_pcm.raw".format(dataName)
file = open(fileName, 'rb')
size = int(os.path.getsize(fileName) / pcmData1.itemsize)
count = int(size)
pcmData1.fromfile(file, size)
file.close()

fileName = "output/LOST_cont_sample4x_freq128x_COMP_none/pcm/MY_{}_pcm.raw".format(dataName)
file = open(fileName, 'rb')
size = int(os.path.getsize(fileName) / pcmData2.itemsize)
count = int(size)
pcmData2.fromfile(file, size)
file.close()

fileName = "output/LOST_cont_sample4x_freq128x_COMP_intp/pcm/MY_{}_pcm.raw".format(dataName)
file = open(fileName, 'rb')
size = int(os.path.getsize(fileName) / pcmData3.itemsize)
count = int(size)
pcmData3.fromfile(file, size)
file.close()

def showPCM(start = 0, end = 500) :
	fig = pyplot.figure(1)
	fig.set_figwidth(15)
	fig.subplots_adjust(left=0.05, right=0.95)
	pyplot.title('{} pcm data'.format(dataName))
	pyplot.plot(range(start, end), pcmData1[start:end], 'g', linewidth=0.5, label='golden')
	pyplot.plot(range(start, end), pcmData2[start:end], 'k', linewidth=0.5, label='lost')
	pyplot.plot(range(start, end), pcmData3[start:end], 'r', linewidth=0.5, label='comp')
	pyplot.legend()
	pyplot.show()

# showPCM(0, int(count/1000))
# showPCM()
