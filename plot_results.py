import matplotlib.pyplot as plt
import re
import numpy as np

def parse_results(filename='results.txt'):
    """Parse the results.txt file and extract benchmark data"""
    data = {
        'AES-128': {'sizes': [], 'throughput': [], 'time': []},
        'SPECK-128': {'sizes': [], 'throughput': [], 'time': []},
        'ASCON-128': {'sizes': [], 'throughput': [], 'time': []}
    }
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    for line in lines:
        # Match lines like: "AES-128      |    256 bytes |      50000 runs |    0.099 s |     123.30 MB/s |     1.98 us/op"
        match = re.search(r'(AES-128|SPECK-128|ASCON-128)\s*\|\s*(\d+)\s*bytes\s*\|\s*\d+\s*runs\s*\|\s*([\d.]+)\s*s\s*\|\s*([\d.]+)\s*MB/s', line)
        if match:
            algo = match.group(1)
            size = int(match.group(2))
            time = float(match.group(3))
            throughput = float(match.group(4))
            
            data[algo]['sizes'].append(size)
            data[algo]['time'].append(time)
            data[algo]['throughput'].append(throughput)
    
    return data

def plot_throughput(data):
    """Plot throughput comparison"""
    plt.figure(figsize=(12, 6))
    
    for algo, values in data.items():
        if values['sizes']:  # Only plot if data exists
            plt.plot(values['sizes'], values['throughput'], 
                    marker='o', linewidth=2, markersize=8, label=algo)
    
    plt.xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
    plt.ylabel('Throughput (MB/s)', fontsize=12, fontweight='bold')
    plt.title('Cryptographic Algorithm Throughput Comparison', fontsize=14, fontweight='bold')
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    plt.tight_layout()
    plt.savefig('throughput_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: throughput_comparison.png")
    plt.show()

def plot_execution_time(data):
    """Plot execution time comparison"""
    plt.figure(figsize=(12, 6))
    
    for algo, values in data.items():
        if values['sizes']:
            plt.plot(values['sizes'], values['time'], 
                    marker='s', linewidth=2, markersize=8, label=algo)
    
    plt.xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
    plt.ylabel('Execution Time (seconds)', fontsize=12, fontweight='bold')
    plt.title('Cryptographic Algorithm Execution Time Comparison', fontsize=14, fontweight='bold')
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    plt.yscale('log')
    plt.tight_layout()
    plt.savefig('execution_time_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: execution_time_comparison.png")
    plt.show()

def plot_bar_chart(data):
    """Plot bar chart for specific message size"""
    # Find a common message size
    common_sizes = set(data['AES-128']['sizes'])
    for algo in ['SPECK-128', 'ASCON-128']:
        common_sizes = common_sizes.intersection(set(data[algo]['sizes']))
    
    if not common_sizes:
        print("No common message sizes found for bar chart")
        return
    
    # Use the largest common size
    target_size = max(common_sizes)
    
    algorithms = []
    throughputs = []
    
    for algo, values in data.items():
        if target_size in values['sizes']:
            idx = values['sizes'].index(target_size)
            algorithms.append(algo)
            throughputs.append(values['throughput'][idx])
    
    plt.figure(figsize=(10, 6))
    colors = ['#ff6b6b', '#4ecdc4', '#45b7d1']
    bars = plt.bar(algorithms, throughputs, color=colors, edgecolor='black', linewidth=1.5)
    
    # Add value labels on bars
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom', fontsize=11, fontweight='bold')
    
    plt.xlabel('Algorithm', fontsize=12, fontweight='bold')
    plt.ylabel('Throughput (MB/s)', fontsize=12, fontweight='bold')
    plt.title(f'Throughput Comparison at {target_size} bytes', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    plt.savefig('throughput_bar_chart.png', dpi=300, bbox_inches='tight')
    print(f"✓ Saved: throughput_bar_chart.png (for {target_size} bytes)")
    plt.show()

def plot_speedup_comparison(data):
    """Plot speedup relative to AES"""
    plt.figure(figsize=(12, 6))
    
    aes_sizes = data['AES-128']['sizes']
    aes_throughput = data['AES-128']['throughput']
    
    for algo in ['SPECK-128', 'ASCON-128']:
        speedup = []
        sizes = []
        
        for i, size in enumerate(data[algo]['sizes']):
            if size in aes_sizes:
                aes_idx = aes_sizes.index(size)
                speedup_factor = data[algo]['throughput'][i] / aes_throughput[aes_idx]
                speedup.append(speedup_factor)
                sizes.append(size)
        
        if sizes:
            plt.plot(sizes, speedup, marker='D', linewidth=2, markersize=8, label=f'{algo} vs AES-128')
    
    plt.axhline(y=1.0, color='red', linestyle='--', linewidth=2, label='AES-128 Baseline')
    plt.xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
    plt.ylabel('Speedup Factor (relative to AES-128)', fontsize=12, fontweight='bold')
    plt.title('Performance Speedup Relative to AES-128', fontsize=14, fontweight='bold')
    plt.legend(fontsize=11)
    plt.grid(True, alpha=0.3)
    plt.xscale('log', base=2)
    plt.tight_layout()
    plt.savefig('speedup_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: speedup_comparison.png")
    plt.show()

def print_summary_table(data):
    """Print a summary table of results"""
    print("\n" + "="*80)
    print(" "*20 + "BENCHMARK RESULTS SUMMARY")
    print("="*80)
    
    for algo, values in data.items():
        if values['sizes']:
            print(f"\n{algo}:")
            print(f"  Message Sizes Tested: {len(values['sizes'])}")
            print(f"  Size Range: {min(values['sizes'])} - {max(values['sizes'])} bytes")
            print(f"  Throughput Range: {min(values['throughput']):.2f} - {max(values['throughput']):.2f} MB/s")
            print(f"  Average Throughput: {np.mean(values['throughput']):.2f} MB/s")
    
    print("\n" + "="*80)

def main():
    print("\n" + "="*80)
    print(" "*15 + "CRYPTOGRAPHY BENCHMARK VISUALIZATION TOOL")
    print("="*80 + "\n")
    
    try:
        # Parse results
        print("📊 Parsing results.txt...")
        data = parse_results('results.txt')
        
        # Check if data was found
        has_data = any(len(values['sizes']) > 0 for values in data.values())
        if not has_data:
            print("\n❌ No benchmark data found in results.txt!")
            print("   Please run benchmark.exe first and test some message sizes.\n")
            return
        
        print(f"✓ Found data for {sum(1 for v in data.values() if v['sizes'])} algorithm(s)\n")
        
        # Print summary
        print_summary_table(data)
        
        # Generate plots
        print("\n📈 Generating plots...\n")
        plot_throughput(data)
        plot_execution_time(data)
        plot_bar_chart(data)
        plot_speedup_comparison(data)
        
        print("\n" + "="*80)
        print("✅ All plots generated successfully!")
        print("="*80 + "\n")
        
    except FileNotFoundError:
        print("\n❌ Error: results.txt not found!")
        print("   Please run benchmark.exe first.\n")
    except Exception as e:
        print(f"\n❌ Error: {e}\n")

if __name__ == "__main__":
    main()