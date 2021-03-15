import numpy as np
import matplotlib.pyplot as plt

plotStuff = False


time = 128            # resolution in Time
curves = 64         # number of curves
maxExp = 1.5        # steepnes of the max curve
B = 4095            # max Val

x = np.linspace(0,1,time)


LUT = np.zeros((curves, time))

stepIdx = 0
print(np.logspace(0.1,maxExp,round(curves/2)))
for exp in np.logspace(0.1,maxExp,round(curves/2)):
    yExp = x**exp
    LUT[:][int(curves/2)-stepIdx-1] = yExp
    LUT[:][int(curves/2)+stepIdx] = abs(np.flip(yExp) - 1)
    stepIdx = stepIdx + 1



LUT = np.around(LUT*B)

f = open("LUT.h", "w")
f.write(f"const int MAX={B};\n")
f.write(f"const int TIME={time};\n")
f.write(f"const int CURVES={curves};\n")

f.write(f"static const PROGMEM uint16_t LUT[{curves}][{time}] = ")
f.write("{")
colPos = 0
for row in LUT:
    f.write("{")
    rowPos = 0
    for val in row:
        if rowPos == time-1:
            endStr = ""
        else:
            endStr = ", "
        f.write('{0:01}'.format(int(val)))
        # f.write('{0:04}'.format(int(val)), end=endStr)
        # f.write('{0:#0{1}x}'.format(int(val),6), end=", ")
        f.write(endStr)
        rowPos += 1
    if colPos == curves-1:
        f.write("}};\n")
    else:
        f.write("},\n")
    colPos += 1

f.close()

if plotStuff == True:
    for stepIdx in range(curves):
        plt.plot(x,LUT[:][stepIdx])
    plt.show()
