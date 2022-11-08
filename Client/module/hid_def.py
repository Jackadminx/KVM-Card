import hid
import time

vendor_id = 0x2B86
usage_page = 0xFFB1
product_id = 0xE6E0


# 初始化HID设备
def init_usb(vendor_id, usage_page):
    global h
    h = hid.device()
    hid_enumerate = hid.enumerate()
    device_path = 0
    for i in range(len(hid_enumerate)):
        if (hid_enumerate[i]['usage_page'] == usage_page and hid_enumerate[i]['vendor_id'] == vendor_id):
            # if (hid_enumerate[i]['usage_page'] == usage_page and hid_enumerate[i]['vendor_id'] == vendor_id and hid_enumerate[i]['product_id'] == product_id ):
            device_path = hid_enumerate[i]['path']
    if (device_path == 0): return "Device not found"
    h.open_path(device_path)
    h.set_nonblocking(1)  # enable non-blocking mode
    return 0


# 读写HID设备
def hid_report(vendor_id, usage_page, buffer, rw_mode=True):
    print("<", buffer)
    try:
        h.write(buffer)
    except (OSError, ValueError):
        print("写入设备错误")
        return 1
    except NameError:
        print("未初始化设备")
        return 4
    if rw_mode:  # 读取回复
        time_start = time.time()
        while 1:
            try:
                d = h.read(64)
            except (OSError, ValueError):
                print("读取数据错误")
                return 2
            if d:
                print(">", d)
                break
            if time.time() - time_start > 3:
                print("超时未回应")
                d = 3
                break
    else:
        d = 0
    return d
