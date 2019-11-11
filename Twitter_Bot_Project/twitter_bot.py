import tweepy
import time
import csv

FILE_NAME = 'lastSeenID.txt'

CONSUMER_KEY = 'MVDkHczOFTe6XveqJWmSpc4x0'
CONSUMER_SECRET = 'H7hCpordbt5k69vNHVBwHwM2vmZ7xV8OIUfoExH7Pg8K3VY1X0'
ACCESS_KEY = '1193655897559633920-Svc7Ofqk1hEpIrJWS2ZgVSkm8nFGMk'
ACCESS_SECRET = 'XvA1hmTDln807bjZCw480xXjWJfXM6BPlPwMybVVl8VUS'

#authentication
auth = tweepy.OAuthHandler(CONSUMER_KEY,CONSUMER_SECRET)
auth.set_access_token(ACCESS_KEY, ACCESS_SECRET)
api = tweepy.API(auth, wait_on_rate_limit=True)
print('This is my twitter bot')



#takes file name and returns the last seen ID
def retrieveLastSeenID(fileName):
    fileRead = open(fileName, 'r')
    lastSeenID = int(fileRead.read().strip())
    fileRead.close()
    return lastSeenID
#takes new last seen ID and fileName and then write new last seen ID to text file
def storeLastSeenID(lastSeenID, fileName):
    fileWrite = open(fileName, 'w')
    fileWrite.write(str(lastSeenID))
    fileWrite.close()
    return

def replyToTweets():
    print('Retrieving and replying to tweets...')
    #DEV note: use 1193669727366893568 (first tweet id) for testing
    lastSeenID = retrieveLastSeenID(FILE_NAME)
    mentions = api.mentions_timeline(lastSeenID, tweet_mode = 'extended')

    for mention in reversed(mentions):
        print(str(mention.id) + ' - ' + mention.full_text)
        lastSeenID = mention.id
        storeLastSeenID(lastSeenID, FILE_NAME)
        if '#helloworld' in mention.full_text.lower():
            print('found #helloworld')
            print('responding back...')
            api.update_status('@' + mention.user.screen_name + ' #HelloWorld back to you!', mention.id)

def locationReply():
    print('Retrieving and replying to tweets...')
    with open('philly_locations.csv') as csvFile:
        csvReader = csv.reader(csvFile, delimiter = ';')
        botDict = dict((rows[0], rows[1]) for rows in csvReader)
        #print(myDict)
        location = [*botDict]
        lastSeenID = retrieveLastSeenID(FILE_NAME)
        mentions = api.mentions_timeline(lastSeenID, tweet_mode = 'extended')
        
        #Starting from the first mention tweet
        for mention in reversed(mentions):
            print('\n' + str(mention.id) + ' - ' + mention.full_text + '\n')
            #The ID number of the mention tweet
            lastSeenID = mention.id
            storeLastSeenID(lastSeenID, FILE_NAME)
            #for each key in the dictionary
            for locationKey in botDict:
                #if the key is found in the tweet
                if locationKey in mention.full_text:
                    print('Found a location in Philly')
                    print('Responding back with the location\'s address')
                    #reply back to the user
                    api.update_status('@' + mention.user.screen_name + ' ' + botDict[locationKey], mention.id)
                    print(botDict[locationKey])


if __name__== "__main__":
    while True:
        #replyToTweets()
        locationReply()
        time.sleep(15)

#DEV note: use 1193669727366893568 (first tweet id) for testing