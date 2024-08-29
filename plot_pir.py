import pandas as pd
import matplotlib.pyplot as plt

para = ["pir_read", "pir_write"]
block_name = ["element_size", "tag_size"]
XList = [1 << i for i in range(6, 21, 2)]

# 设置图像的显示参数
plt.rcParams.update({
    "legend.fancybox": False,
    "legend.frameon": True,
    "font.serif": ["Times"], # Times New Roman
    "font.size": 23,
    "axes.titlesize": 23, # Title font size
    "axes.labelsize": 23  # Label font size
})

# 创建三张并列的子图
fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 6))  # 更大的figsize适合三图并列

markL = ['X', 'o', 's', 'd', 'p']
colorL = ['red', 'lime', 'blue', 'orange', 'purple']


csv_file = 'TW-PIR/Result/{}_results.csv'.format(para[0])
df = pd.read_csv(csv_file) # df['log_database_size']
element_sizes = df[block_name[0]].unique()

for i, size in enumerate(element_sizes):
    subset = df[df[block_name[0]] == size]
    if i == 0:
        label_prefix = 'Block Size: '
    else:
        label_prefix = ''
    ax1.plot(XList, subset['transfer_bytes (B)'] / 1024, 
             marker=markL[i], color=colorL[i], linestyle='dashed', 
             label=f'{label_prefix}{size} bytes', markersize=15)

ax1.set_title('Bandwidth Consumption', pad=10)
ax1.set_xticks(XList)
ax1.set_xscale('log', base=2)
ax1.set_yscale('log', base=2)
ax1.set_xlabel('Database Size (# of blocks)')
ax1.set_ylabel('Bandwidth (KB)')
ax1.grid(True)

for i, size in enumerate(element_sizes):
    subset = df[df[block_name[0]] == size]
    ax2.plot(XList, subset['buildT (s)'] * 1000, 
            marker=markL[i], color=colorL[i], linestyle='dashed', 
            label=f'{size} bytes', markersize=15)

ax2.set_title('PIR Read', pad=10)
ax2.set_xticks(XList)
ax2.set_xscale('log', base=2)
ax2.set_yscale('log', base=2)
ax2.set_xlabel('Database Size (# of blocks)')
ax2.set_ylabel('Time (ms)')
ax2.grid(True)

csv_file = 'TW-PIR/Result/{}_results.csv'.format(para[1])
df = pd.read_csv(csv_file) # df['log_database_size']
element_sizes = df[block_name[1]].unique()

for i, size in enumerate(element_sizes):
    subset = df[df[block_name[1]] == size]
    ax3.plot(XList, subset['buildT (s)'] * 1000, 
            marker=markL[i], color=colorL[i], linestyle='dashed', 
            label=f'{size} bytes', markersize=15)
    
ax3.set_title('PIR Write', pad=10)
ax3.set_xticks(XList)
ax3.set_xscale('log', base=2)
ax3.set_yscale('log', base=2)
ax3.set_xlabel('Database Size (# of blocks)')
ax3.set_ylabel('Time (ms)')
ax3.grid(True)


handles, labels = ax1.get_legend_handles_labels()
fig.legend(handles, labels, loc='upper center', ncol=5, bbox_to_anchor=(0.5, 0.96))

plt.tight_layout(rect=[0, 0, 1, 0.88]) 
plt.savefig('pir_cost.pdf', bbox_inches = 'tight', dpi = 600)
plt.close()
plt.show()
