import random
import sys

instruments = ['AAPL', 'GOOG', 'NVDA']
i_len = len(instruments)

action = ['B', 'S']

'''
generate (count // 2) buys, then a cancel, then (count // 2) sells. The instrument is randomly chosen from instruments
file_handle: the file
id: client id
start: the starting order id
count: # of orders to generate
'''
def client_write(file_handle, id, start, count):
    for i in range(count // 2):
        file_handle.write("%d B %d %s %d %d\n" % (id, start + i, instruments[random.randint(0, i_len - 1)], 100, 10))

    file_handle.write(f"{id} C {start}\n")
    for i in range(count // 2):
        file_handle.write("%d S %d %s %d %d\n" % (id, start + count // 2 + i, instruments[random.randint(0, 1)], 100, 1))


'''
this time action (buy or sell), instrument, price and count are all randomly chosen
then cancel the 2nd half of buy/sell orders
'''
def client_write2(file_handle, id, start, count):
    for i in range(count):
        file_handle.write(f'{id} {action[random.randint(0, 1)]} {start + i} {instruments[random.randint(0, i_len - 1)]} ' 
                          f'{random.randint(1, 100)} ' + 
                          f'{random.randint(1, 100)}\n')
        
    
    for i in range(count // 2):
        file_handle.write(f'{id} C {start + count // 2 + i}\n')


def create_testcase(num_threads, num_orders_per_thread):
    with open('tests/test.in', 'w') as file:
        file.write(f'{num_threads}\n')
        file.write('o\n\n')

        for i in range(num_threads):
            client_write2(file, i, (i + 1) * 2 * num_orders_per_thread, num_orders_per_thread)
            file.write('\n')

        file.write('x\n')
    
create_testcase(int(sys.argv[1]), int(sys.argv[2]))